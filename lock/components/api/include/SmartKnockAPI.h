#include <string.h>

#include "esp_http_client.h"

struct ServerMessage {};

struct LockMessage {};

class SmartKnockAPI {
   public:
    static constexpr const char* a = "hi";
    char* url = nullptr;
    uint32_t url_len = 0;
    void get_incoming_messages(ServerMessage*& arr_out, int& num_out);
    void send_message(const LockMessage& msg);

   private:
    static esp_err_t _http_event_handler(esp_http_client_event_t* evt);
};