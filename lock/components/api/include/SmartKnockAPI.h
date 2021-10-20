#include <string.h>

#include <string>
#include <sstream>
#include "esp_http_client.h"

enum MessageType { NONE, LOCK, UNLOCK };

struct ServerMessage {
    int id;
    MessageType type;
};

struct LockMessage {};

class SmartKnockAPI {
   public:
    static constexpr const char* api_base_url = "https://smartknock.xyz/";
    static constexpr const int http_response_max_size = 1024;
    char* url = nullptr;
    uint32_t url_len = 0;
    MessageType get_incoming_message();
    void send_message(const LockMessage& msg);

   private:
    int last_consumed_message_id = 0;
    std::string make_http_get_request(const char* url);
    static esp_err_t _http_event_handler(esp_http_client_event_t* evt);
};