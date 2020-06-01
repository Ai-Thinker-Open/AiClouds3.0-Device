// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/param.h>
#include "esp_system.h"
#include "esp8266/eagle_soc.h"

extern unsigned int adc_rand_noise;
uint32_t esp_random(void)
{
    return (adc_rand_noise ^ REG_READ(WDEV_RAND));
}

void esp_fill_random(void *buf, size_t len)
{
    assert(buf != NULL);
    uint8_t *buf_bytes = (uint8_t *)buf;
    while (len > 0) {
        uint32_t word = esp_random();
        uint32_t to_copy = MIN(sizeof(word), len);
        memcpy(buf_bytes, &word, to_copy);
        buf_bytes += to_copy;
        len -= to_copy;
    }
}
