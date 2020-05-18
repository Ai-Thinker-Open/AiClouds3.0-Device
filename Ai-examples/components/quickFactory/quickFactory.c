#include "common.h"
#include "quickFactory.h"

static const char *TAG = "AIThinker-quickFactory";

void clean_flag_quickBoot()
{
    nvs_handle out_handle;
    int8_t nvs_i8 = 0;
    if (nvs_open("innet_conf", NVS_READWRITE, &out_handle) == ESP_OK)
    {
        //设置为0
        ESP_ERROR_CHECK(nvs_set_i8(out_handle, "innet_conf", nvs_i8));
    }
    nvs_close(out_handle);
}

int funtion_add_flag_quickBoot()
{
    nvs_handle out_handle;
    int8_t nvs_i8 = 0;

    if (nvs_open("innet_conf", NVS_READWRITE, &out_handle) != ESP_OK)
    {
        ESP_LOGI(TAG, "open innet_conf fail\n");
        return nvs_i8;
    }

    //读取，类似数据读字段对应的值
    esp_err_t err = nvs_get_i8(out_handle, "innet_conf", &nvs_i8);

    if (err == ESP_OK)
    {
        nvs_i8++;
    }

    err = nvs_set_i8(out_handle, "innet_conf", nvs_i8);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "nvs_i8 Error");

    nvs_close(out_handle);

    return nvs_i8;
}

int funtion_set_flag_quickBoot()
{
    nvs_handle out_handle;
    int8_t nvs_i8 = 5;

    if (nvs_open("innet_conf", NVS_READWRITE, &out_handle) != ESP_OK)
    {
        ESP_LOGI(TAG, "open innet_conf fail\n");
        return nvs_i8;
    }

    //读取，类似数据读字段对应的值
    esp_err_t err = nvs_set_i8(out_handle, "innet_conf", nvs_i8);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "nvs_i8 Error");

    nvs_close(out_handle);

    return nvs_i8;
}