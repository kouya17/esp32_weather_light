#include <WiFi.h>
#include <WiFiServer.h>
#include <map>
 
typedef void (*Handler_t)(void) ;
typedef void (*Handler_t_post)(String) ;
typedef void (*DefaultHandler_t)(String) ;
typedef struct {
  int status;
  const char* cpDesc;
} response_t;
 
const response_t responses[] = {
  {200, "200 OK"},
  {404, "404 Not Found"},
  {500, "500 Server Error"},
  {0, 0}
};
 
class SimpleWebServer : public WiFiServer {
  IPAddress ap_addr;
  IPAddress ap_netmask;
  String ssid;
  String password;
  std::map<String, Handler_t> request_handlers;
  std::map<String, Handler_t_post> request_handlers_post;
  DefaultHandler_t default_handler = 0;
  WiFiClient* client_connected;
 
public:
  SimpleWebServer(const char* ssid, const char* password, const IPAddress ap_addr, const IPAddress ap_netmask, int port = 80) : WiFiServer(port) {
    this->ap_addr = ap_addr;
    this->ap_netmask = ap_netmask;
    this->ssid = ssid;
    this->password = password;
    client_connected = NULL;
  }
 
  void begin() {
    WiFi.softAP(ssid.c_str(), password.c_str());
    WiFi.softAPConfig(ap_addr, ap_addr, ap_netmask);
    Serial.print("SoftAP : "); 
    Serial.print(WiFi.softAPIP());
    Serial.println(" " + ssid + String(" starts.")); 
    WiFiServer::begin();
    Serial.println("SimpleWebServer begin()");
  }
 
  void add_handler(String uri, Handler_t func) {
    request_handlers[uri] = func;
  }

  void add_handler_post(String uri, Handler_t_post func) {
    request_handlers_post[uri] = func;
  }
 
  void add_default_handler(DefaultHandler_t func) {
    default_handler = func;
  }
 
  void send_response(const char* cp)
  {
    if (client_connected)
      client_connected->print(cp);
  }

  void send_html(int status, const char* cp)
  {
    String s = "HTTP/1.1 " + String(status) + " OK\r\nContent-type:text/html\r\n\r\n";
    s = s + String(cp);
    send_response(s.c_str());
  }
 
  void send_status(int status, const char* cp = NULL)
  {
    if (!client_connected)
      return;
    const response_t* p = responses;
    while(p->status != 0) {
      if (p->status == status) {
        String s = "HTTP/1.1 " + String(status) + String(" ") + String(p->cpDesc) + String("Content-type:text/html\r\n\r\n");
        const char* cpDesc = cp ? cp : p->cpDesc;
        s = s + String("<HTML><BODY style='font-size:48px;'>") + String(cpDesc) + String("</BODY></HTML>\r\n");
        client_connected->print(s);
        return;        
      }
      p++;
    }
    send_status(200);
  }
 
  void handle_request() {
    WiFiClient client = available();
    if (!client)
      return;
 
    String request_line = "";
    String request_method = "";
    while (client) {
      client_connected = &client;
      while(client.available()) {
          String line = client.readStringUntil('\r');
          line.trim();
          if (line.startsWith("GET")) {
            request_method = "GET";
            int index = line.indexOf(" ", 5);
            if (index > 0)
              request_line = line.substring(4, index);
            else
              request_line = line.substring(4);
            request_line.trim();
            continue;
          }else if (line.startsWith("POST")) {
            request_method = "POST";
            int index = line.indexOf(" ", 5);
            if (index > 0)
              request_line = line.substring(4, index);
            else
              request_line = line.substring(4);
            request_line.trim();
            continue;
          }
          if (line.length() == 0) { // end of request headers.
            Serial.println("request_line = " + request_line);
            if(request_method == "GET") {
              Handler_t f = request_handlers[request_line];
              if (!f) {
                if (default_handler)
                  default_handler(request_line);
                else
                  send_status(404);
              } else
                f();
              client.flush();
            }else if(request_method == "POST") {
              line = client.readStringUntil('\r');
              line.trim();
              Handler_t_post f = request_handlers_post[request_line];
              if (!f) {
                if (default_handler)
                  default_handler(request_line);
                else
                  send_status(404);
              } else
                f(line);
              client.flush();
            }
          }
      }
      client_connected = NULL;
      client.stop();
    }
  }
};
