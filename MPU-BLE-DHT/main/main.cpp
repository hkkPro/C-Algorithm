#include <stdio.h>
// #include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#define SDA GPIO_NUM_8
#define SCL GPIO_NUM_9
#define DHT_IO GPIO_NUM_4

int64_t timeout = 0;

static int16_t current_temp_ble = 0;
static uint16_t current_hum_ble = 0;
static uint8_t own_addr_type;

static const ble_uuid16_t sensor_measuremenets_svc_uuid = BLE_UUID16_INIT(0x181A);
static const ble_uuid16_t temp_uuid_chr = BLE_UUID16_INIT(0x2A6E);
static const ble_uuid16_t hum_uuid_chr = BLE_UUID16_INIT(0x2A6F);

static uint16_t temp_val_handle;
static uint16_t hum_val_handle;

static int temp_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int hum_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int ble_gap_event_cb(struct ble_gap_event *event, void *arg);
static void ble_app_advertise(void);

static const struct ble_gatt_chr_def gatt_svr_chrs[] = {
    {
        .uuid = &temp_uuid_chr.u,
        .access_cb = temp_chr_access,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &temp_val_handle,
    },
    {
        .uuid = &hum_uuid_chr.u,
        .access_cb = hum_chr_access,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &hum_val_handle,
    },
    {
        0,
    }
};

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &sensor_measuremenets_svc_uuid.u,
        .characteristics = gatt_svr_chrs,
    },
    {
        0,
    },
};

// i2c_master_bus_config_t master = { 
// .i2c_port = -1,
// .sda_io_num = SDA,
// .scl_io_num = SCL,
// .clk_source = I2C_CLK_SRC_DEFAULT,

// };

// i2c_master_bus_handle_t master_handle;

static int temp_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
    {
        int rc = os_mbuf_append(ctxt->om, &current_temp_ble, sizeof(current_temp_ble));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int hum_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
    {
        int rc = os_mbuf_append(ctxt->om, &current_hum_ble, sizeof(current_hum_ble));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields = {0};
    const char *name = ble_svc_gap_device_name();

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
}

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    if (event->type == BLE_GAP_EVENT_CONNECT)
    {
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
    }
    else if (event->type == BLE_GAP_EVENT_DISCONNECT)
    {
        ble_app_advertise();
    }
    return 0;
}

static void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &own_addr_type);
    ble_app_advertise();
}

static void nimble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t DHT11_CFG(void)
{
    gpio_set_direction(DHT_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(DHT_IO, 1);
    gpio_set_direction(DHT_IO, GPIO_MODE_INPUT);
    timeout = esp_timer_get_time();
    while (gpio_get_level(DHT_IO) == 1)
    {
        if (esp_timer_get_time() - timeout > 90) return ESP_ERR_TIMEOUT;
    }
    timeout = esp_timer_get_time();
    while (gpio_get_level(DHT_IO) == 0)
    {
        if (esp_timer_get_time() - timeout > 120) return ESP_ERR_TIMEOUT;
    }
    timeout = esp_timer_get_time();
    while (gpio_get_level(DHT_IO) == 1)
    {
        if (esp_timer_get_time() - timeout > 120) return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t DHT_read(uint8_t *DHT_data)
{
    for (int i = 0; i < 40; i++)
    {
        timeout = esp_timer_get_time();
        while (gpio_get_level(DHT_IO) == 0)
        {
            if (esp_timer_get_time() - timeout > 80) return ESP_ERR_TIMEOUT;
        }
        timeout = esp_timer_get_time();
        while (gpio_get_level(DHT_IO) == 1)
        {
            if (esp_timer_get_time() - timeout > 120) return ESP_ERR_TIMEOUT;
        }
        int64_t elapsed = esp_timer_get_time() - timeout;
        DHT_data[i / 8] <<= 1;
        if (elapsed > 40)
        {
            DHT_data[i / 8] |= 1;
        }
    }
    return ESP_OK;
}

void nimBLE(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK)
    {
        ESP_LOGE("NimBLE", "Failed to initialize NVS, error code: %d", ret);
        return;
    }

    ret = nimble_port_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE("NimBLE", "NimBLE port initialization failed, code: %d", ret);
        return;
    }

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    ble_svc_gap_init();
    int rc = ble_svc_gap_device_name_set("ESP32");
    if (rc != 0)
    {
        ESP_LOGE("NimBLE", "Failed to set device name, error code: %d", rc);
        return;
    }

    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    nimble_port_freertos_init(nimble_host_task);
}

extern "C" void app_main(void)
{
    // ESP_ERROR_CHECK(i2c_new_master_bus(&master, &master_handle));
    vTaskDelay(pdMS_TO_TICKS(1000));
    uint8_t DHT_data[5] = {0};
    nimBLE();
    while (true)
    {
        if (DHT11_CFG() != ESP_OK)
        {
            printf("Handshake failed, can't read from DHT11\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        if (DHT_read(DHT_data) != ESP_OK)
        {
            printf("Reading failed (Read timeout...)\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        else
        {
            if (((DHT_data[0] + DHT_data[1] + DHT_data[2] + DHT_data[3]) & 0xFF) == DHT_data[4])
            {
                printf("Wilgotnosc: %d.%d%%, Temperatura: %d.%d C\n", DHT_data[0], DHT_data[1], DHT_data[2], DHT_data[3]);
                current_hum_ble = (DHT_data[0] * 100) + (DHT_data[1] * 10);
                current_temp_ble = (DHT_data[2] * 100) + (DHT_data[3] * 10);
                ble_gatts_chr_updated(temp_val_handle);
                ble_gatts_chr_updated(hum_val_handle);
            }
            else
            {
                ESP_LOGE("DHT11", "Control sum error!");
            }
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}