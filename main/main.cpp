#include <stdio.h>
#include <math.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define SDA GPIO_NUM_8
#define SCL GPIO_NUM_9

i2c_master_bus_config_t bus = {
    .i2c_port = -1,
    .sda_io_num = SDA,
    .scl_io_num = SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
};
i2c_master_bus_handle_t bus_handle;

i2c_device_config_t mpu6050 {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x68,
    .scl_speed_hz = 400000,
    .scl_wait_us = 0,
};
i2c_master_dev_handle_t mpu_handle;


typedef struct{
    int16_t x;
    int16_t y;
    int16_t z;

}gyro_data_t;

gyro_data_t gyro_parse(const uint8_t *buff_v)
{
    gyro_data_t result;
    result.x = (int16_t)((buff_v[0] << 8) | buff_v[1]);
    result.y = (int16_t)((buff_v[2] << 8) | buff_v[3]);
    result.z = (int16_t)((buff_v[4] << 8) | buff_v[5]);
    return result;
}

extern "C" void app_main(void)
{
    uint8_t gyro_buf[6], accel_buf[6] = {0};
    uint8_t gyro_reg_start = 0x43;
    uint8_t accel_reg_start = 0x3B;
    uint8_t write_buff[2] = {0x6B, 0x00};

    const float rad_na_stopnie = 180.0f / 3.14159265f;
    float kat_z = 0.0f;
    int64_t poprzedni_czas_us = 0;

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus, &bus_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &mpu6050, &mpu_handle));
    ESP_ERROR_CHECK(i2c_master_transmit(mpu_handle, write_buff, sizeof(write_buff), 100));

    while(true)
    {
        esp_err_t err_a = i2c_master_transmit_receive(mpu_handle, &accel_reg_start, sizeof(accel_reg_start), accel_buf, sizeof(accel_buf), 100);
        esp_err_t err_g = i2c_master_transmit_receive(mpu_handle, &gyro_reg_start, sizeof(gyro_reg_start), gyro_buf, sizeof(gyro_buf), 100);

        if (err_a == ESP_OK && err_g == ESP_OK)
        {
            int64_t teraz_us = esp_timer_get_time();

            gyro_data_t g = gyro_parse(gyro_buf);
            gyro_data_t a = gyro_parse(accel_buf);

            // Kąty wokół X i Y z akcelerometru
            float kat_x = atan2f((float)a.y, (float)a.z) * rad_na_stopnie;
            float kat_y = atan2f(-(float)a.x, hypotf((float)a.y, (float)a.z))
                          * rad_na_stopnie;

            // Przyrost kąta wokół Z z żyroskopu
            if (poprzedni_czas_us != 0)
            {
                float dt = (teraz_us - poprzedni_czas_us) / 1000000.0f;
                float predkosc_z = g.z / 131.0f;
                kat_z += predkosc_z * dt;
            }
            poprzedni_czas_us = teraz_us;

            printf("Xa: %d, Ya: %d, Za: %d\n", a.x, a.y, a.z);
            printf("Xg: %d, Yg: %d, Zg: %d\n", g.x, g.y, g.z);
            printf("Katy: X = %.1f°, Y = %.1f°, Z = %.1f°\n",
                   kat_x, kat_y, kat_z);

            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {
            printf("Can't read from accelerometer or gyroscope...\n");
            poprzedni_czas_us = 0;
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}