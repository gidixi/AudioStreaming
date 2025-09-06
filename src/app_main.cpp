extern "C" {
  #include "nvs_flash.h"
  #include "esp_event.h"
  #include "esp_netif.h"
  #include "esp_wifi.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/event_groups.h"
  void app_main();
}
#include <cstring>
#include <cstdio>

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
#define SERVER_PORT 50000
#endif

static EventGroupHandle_t s_wifi_group;
static const int WIFI_CONNECTED_BIT = BIT0;

static void wifi_handler(void*, esp_event_base_t base, int32_t id, void*) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) esp_wifi_connect();
  else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) esp_wifi_connect();
  else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) xEventGroupSetBits(s_wifi_group, WIFI_CONNECTED_BIT);
}

static void wifi_connect() {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  s_wifi_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_handler, nullptr, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_handler, nullptr, nullptr));

  wifi_config_t wc = {};
  std::strncpy((char*)wc.sta.ssid, WIFI_SSID, sizeof(wc.sta.ssid));
  std::strncpy((char*)wc.sta.password, WIFI_PASS, sizeof(wc.sta.password));
  wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
  ESP_ERROR_CHECK(esp_wifi_start());

  xEventGroupWaitBits(s_wifi_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
}

int client_main(int, char**);

extern "C" void app_main() {
  wifi_connect();
  char cid[12];  std::snprintf(cid,  sizeof(cid),  "%d", (int)CLIENT_ID);
  char prt[12];  std::snprintf(prt,  sizeof(prt),  "%d", (int)SERVER_PORT);
  const char* argv[] = {"opus_client", cid, SERVER_IP, prt};
  client_main(4, (char**)argv);
}
