#include "SmartKnockAPI.h"

#include <stdio.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define TAG "api"

MessageType SmartKnockAPI::get_incoming_message() {
    std::string url = SmartKnockAPI::api_base_url;
    uint8_t mac_address[6];
    char hex_address[64];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac_address));
    for (int i = 0; i < 6; i++) sprintf(&hex_address[i * 2], "%02x", mac_address[i]);
    ESP_LOGI(TAG, "mac address: %s", hex_address);

    url += hex_address;
    url += "?format=spaces";
    // ESP_LOGI(TAG, "url: %s", url.c_str());
    auto result = make_http_get_request(url.c_str());
    // ESP_LOGI(TAG, "got: %s", result.c_str());

    std::istringstream response(result);
    int message_id;
    std::string type;
    while (response >> message_id >> type) {
        if (message_id > last_consumed_message_id) {
            last_consumed_message_id = message_id;
            if (type == "LOCK") {
                return MessageType::LOCK;
            } else if (type == "UNLOCK") {
                return MessageType::UNLOCK;
            }
        }
    }

    return MessageType::NONE;
}

void SmartKnockAPI::send_message(const LockMessage &msg) {}

std::string SmartKnockAPI::make_http_get_request(const char *url) {
    char local_response_buffer[SmartKnockAPI::http_response_max_size] = {0};

    esp_http_client_config_t config{};
    config.url = url;
    config.event_handler = _http_event_handler;
    config.user_data = local_response_buffer;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        // ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
        //          esp_http_client_get_status_code(client),
        //          esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    // ESP_LOGI(TAG, "Received: %s", local_response_buffer);
    // ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));
    return local_response_buffer;
}

esp_err_t SmartKnockAPI::_http_event_handler(esp_http_client_event_t *evt) {
    static char
        *output_buffer;     // Buffer to store response of http request from event handler
    static int output_len;  // Stores number of bytes read
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
                     evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used
             * in this example returns binary data. However, event handler can also be
             * used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy((char *)evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *)malloc(
                            esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to
                // print the accumulated response ESP_LOG_BUFFER_HEX(TAG, output_buffer,
                // output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            // int mbedtls_err = 0;
            // esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err,
            // NULL); if (err != 0) {
            //     if (output_buffer != NULL) {
            //         free(output_buffer);
            //         output_buffer = NULL;
            //     }
            //     output_len = 0;
            //     ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            //     ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            // }
            break;
    }
    return ESP_OK;
}