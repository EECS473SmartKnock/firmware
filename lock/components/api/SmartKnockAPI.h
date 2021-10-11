#include <string>

#include "esp_http_client.h"

struct ServerMessage {};

struct LockMessage {};

class SmartKnockAPI {
   public:
    static constexpr const char* a = "hi";
    std::string url = "hi";
    void get_incoming_messages(ServerMessage*& arr_out, int& num_out);
    void send_message(const LockMessage& msg);

   private:
    static esp_err_t _http_event_handler(esp_http_client_event_t* evt);
};