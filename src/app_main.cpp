// src/app_main.cpp
extern "C" {
  #include "nvs_flash.h"
  #include "esp_event.h"
  #include "esp_netif.h"
  #include "esp_wifi.h"
  #include "esp_log.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/event_groups.h"
  void app_main();
}

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_SSID"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "YOUR_PASS"
#endif

#ifndef CLIENT_ID
#define CLIENT_ID 1
#endif
#ifndef SERVER_IP
#define SERVER_IP "192.168.1.100"
#endif
#ifndef SERVER_PORT
#define SERVER_PORT 9000
#endif

static const char* TAG = "APP";
static EventGroupHandle_t s_wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect(); // retry
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

static void wifi_init_and_connect() {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  s_wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler, nullptr, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler, nullptr, nullptr));

  wifi_config_t wifi_config = {};
  std::strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
  std::strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Connessione Wi-Fi a SSID=%s ...", WIFI_SSID);
  xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  ESP_LOGI(TAG, "Wi-Fi connesso, ho IP.");
}

int client_main(int argc, char** argv); // dichiarata nel tuo client

extern "C" void app_main() {
  wifi_init_and_connect();

  // prepara argv dai define
  const char* id_str = "ID";
  char cid[12]; snprintf(cid, sizeof(cid), "%d", (int)CLIENT_ID);
  const char* argv[] = {"opus_client", cid, SERVER_IP, /*porta*/ nullptr};
  char port_str[12]; snprintf(port_str, sizeof(port_str), "%d", (int)SERVER_PORT);
  argv[3] = port_str;

  client_main(4, (char**)argv);
}
