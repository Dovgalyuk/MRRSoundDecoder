/* Async Request Handlers HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <esp_http_server.h>
#include "cJSON.h"

#include "engine.h"
#include "project.h"

static const char *TAG = "http";

#define MAX_FILE_SIZE   (14*1024*1024)
#define SCRATCH_BUFSIZE 4096
static char scratch[SCRATCH_BUFSIZE];

static esp_err_t web_index_handler(httpd_req_t *req)
{
    extern const unsigned char index_start[] asm("_binary_index_html_start");
    extern const unsigned char index_end[]   asm("_binary_index_html_end");
    const size_t index_size = (index_end - index_start);

    httpd_resp_send_chunk(req, (const char *)index_start, index_size);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t web_control_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, scratch + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    scratch[total_len] = '\0';

    cJSON *root = cJSON_Parse(scratch);
    cJSON *act = cJSON_GetObjectItem(root, "action");
    cJSON *val = cJSON_GetObjectItem(root, "value");
    cJSON *index = cJSON_GetObjectItem(root, "index");
    if (act && act->valuestring) {
        if (val) {
            if (!strcmp(act->valuestring, "set_throttle")) {
                engine_set_throttle(val->valueint);
            } else if (!strcmp(act->valuestring, "function") && index) {
                project_set_function(index->valueint, val->valueint);
            }
        } else if (!strcmp(act->valuestring, "stop")) {
            engine_stop();
        } else if (!strcmp(act->valuestring, "brake")) {
            engine_brake();
        }
    }
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t web_status_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "speed", engine_get_speed());
    cJSON *arr = cJSON_AddArrayToObject(root, "functions");
    for (int i = 0 ; i < PROJECT_FUNCTIONS ; ++i) {
        cJSON *item = cJSON_CreateBool(project_get_function_status(i));
        cJSON_AddItemToArray(arr, item);
    }
    if (cJSON_PrintPreallocated(root, scratch, SCRATCH_BUFSIZE - 10, false)) {
        httpd_resp_sendstr(req, scratch);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON formatting error");
        return ESP_FAIL;
    }
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t web_info_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", project_get_name());
    cJSON *arr = cJSON_AddArrayToObject(root, "functions");
    for (int i = 0 ; i < PROJECT_FUNCTIONS ; ++i) {
        const char *name = project_get_function_name(i);
        if (name) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "function", i);
            cJSON_AddStringToObject(item, "name", name);
            cJSON_AddItemToArray(arr, item);
        }
    }
    if (cJSON_PrintPreallocated(root, scratch, SCRATCH_BUFSIZE - 10, false)) {
        httpd_resp_sendstr(req, scratch);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON formatting error");
        return ESP_FAIL;
    }
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t web_project_upload_handler(httpd_req_t *req)
{
    /* File cannot be larger than a limit */
    if (req->content_len > MAX_FILE_SIZE) {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "File too large");
        return ESP_FAIL;
    }

    int received;
    while ((received = httpd_req_recv(req, scratch, 4)) <= 0) {
        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            /* Retry if timeout occurred */
            continue;
        }
        ESP_LOGE(TAG, "File reception failed!");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
        return ESP_FAIL;
    }

    /* Check signature */
    if (received != 4) {
        ESP_LOGE(TAG, "Signature check failed!");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to check signature");
        return ESP_FAIL;
    }

    project_close();

    FILE *f = fopen(PROJECT_FILENAME, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create file : %s", PROJECT_FILENAME);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }
    if (fwrite(scratch, 1, 4, f) != 4) {
        goto error;
    }

    int remaining = req->content_len - received;
    while (remaining > 0) {
        if ((received = httpd_req_recv(req, scratch, MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }
            goto error;
        }
        if (received && (received != fwrite(scratch, 1, received, f))) {
            goto error;
        }
        remaining -= received;
    }
//ok:
    fclose(f);
    project_open();
    /* Redirect onto root */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
error:
    /* In case of unrecoverable error, close and delete the unfinished file*/
    fclose(f);
    unlink(PROJECT_FILENAME);
    ESP_LOGE(TAG, "File reception failed!");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
    return ESP_FAIL;
}

void web_init(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGI(TAG, "Error starting server!");
        return;
    }

    ESP_LOGI(TAG, "Registering URI handlers");
    const httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = web_index_handler,
    };
    httpd_register_uri_handler(server, &index_uri);

    httpd_uri_t control_post_uri = {
        .uri = "/api/control",
        .method = HTTP_POST,
        .handler = web_control_handler,
    };
    httpd_register_uri_handler(server, &control_post_uri);

    const httpd_uri_t status_uri = {
        .uri       = "/api/status",
        .method    = HTTP_GET,
        .handler   = web_status_handler,
    };
    httpd_register_uri_handler(server, &status_uri);

    const httpd_uri_t info_uri = {
        .uri       = "/api/info",
        .method    = HTTP_GET,
        .handler   = web_info_handler,
    };
    httpd_register_uri_handler(server, &info_uri);

    httpd_uri_t project_upload_post_uri = {
        .uri = "/api/project/upload",
        .method = HTTP_POST,
        .handler = web_project_upload_handler,
    };
    httpd_register_uri_handler(server, &project_upload_post_uri);
}
