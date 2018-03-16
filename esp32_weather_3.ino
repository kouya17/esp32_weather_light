#include "FS.h"
#include <SPIFFS.h>
#include "SimpleWebServer.h"   // webサーバ用ライブラリ(機能esp02ほぼ同等)
#include <map>                 // map型を使用するため
#include <Preferences.h>      // 不揮発領域管理用ライブラリ

// メイン状態変数
/*
#define MAIN_STATE_NONE     0
#define MAIN_STATE_SETTING  1
#define MAIN_STATE_INFORM   2
#define MAIN_STATE_DEFAULT  MAIN_STATE_INFORM
*/
#define DELAY_MAIN_LOOP     1000      // メインloopの実行間隔[ms]
#define MAX_TRY_CONNECT       50      // 最大WiFi接続試行回数
#define MAX_TRY_CONNECT_INI   10      // 最大WiFi接続試行回数(初期接続)

// pin番号
#define PIN_LED_WHITE     25
#define PIN_LED_RED       26
#define PIN_LED_BLUE      23
#define PIN_LED_GREEN      4

// 取ってくる天気の数
#define NUM_GET_WEATHER    6

// webサーバの生成
SimpleWebServer server("ESP32AP", "12345678", IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0), 80);

// 不揮発領域アクセス用クラス
Preferences preferences;

// htmlデータ
uint8_t root_html[16384];         // /index.html
uint8_t success_html[16384];      // /success.html
uint8_t access_html[16384];      // /success.html
uint8_t setinfo_html[16384];      // /success.html
  
void setup(){
  //const char* access_ssid      = "4CE676F828AC_G";              // 無線接続先のssid
  //const char* access_password  = "upvx5vphstsmw";               // 無線接続先のパスワード
  //const char* access_ssid            = "航也 の iPhone";
  //const char* access_password        = "suradann0";
  int connect_try_count = 0;
  
  // シリアルポート開放
  Serial.begin(115200);
  delay(10);

  // LED関係ピン初期化
  pinMode(PIN_LED_WHITE, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  digitalWrite(PIN_LED_WHITE, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);

  // 不揮発性領域へのアクセス開始
  preferences.begin("ssid", false);
  preferences.begin("pass", false);

  // webサーバ設定処理
  set_server();

  // 一度記憶情報を用いてWiFi接続を試みる
  WiFi.begin(preferences.getString("ssid", "").c_str(), preferences.getString("pass", "").c_str());

  while (WiFi.status() != WL_CONNECTED && connect_try_count < MAX_TRY_CONNECT_INI) {
    delay(500);
    Serial.print(".");
    connect_try_count++;
  }

  if (WiFi.status() == WL_CONNECTED) {  
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect process time out.");
  }
}

void loop(){
  //static int main_state = MAIN_STATE_DEFAULT;
  // 天気情報通知間隔設定用変数
  static int count_for_inform_weather = 0;
  static const int count_to_flush_inform_weather = 1; //debug 
  //static const int count_to_flush_inform_weather = 1800; 

  // esp32へのリクエストを処理する
  server.handle_request();
  /*
  switch(main_state){
    case MAIN_STATE_SETTING:
      break;
    case MAIN_STATE_INFORM:
      inform_weather();
      break;
    case MAIN_STATE_NONE:
      break;
  }
  */
  // ループ回数が規定値を超えるごとに天気情報通知
  if(count_for_inform_weather >= count_to_flush_inform_weather){
    // 天気情報通知処理
    inform_weather();
    // ループ回数を初期化
    count_for_inform_weather = 0;
  }

  // ループ回数を１回増やす
  count_for_inform_weather++;
  delay(DELAY_MAIN_LOOP);
}

/*-------------------------------------------------*/
// webサーバ設定処理
/*-------------------------------------------------*/
void set_server(){
  // パス設定
  const char *root_path = "/index.html";
  const char *success_path = "/success.html";
  const char *access_path = "/access.html";
  const char *setinfo_path = "/setinfo.html";
  // htmlファイル読み込み用変数
  File html_file;
  size_t size;
  //const char* soft_ap_ssid     = "weather_bot";                 // ソフトAPのssid
  //const char* soft_ap_password = "esp32esp";                    // ソフトAPのパスワード

  // esp32に記憶させてあるhtmlファイルを読み込んでいく
  // index.htmlを読み込む
  SPIFFS.begin();
  html_file = SPIFFS.open(root_path, "r");
  size = html_file.size();
  html_file.read(root_html, size);
  html_file.close();

  // success.htmlを読み込む
  html_file = SPIFFS.open(success_path, "r");
  size = html_file.size();
  html_file.read(success_html, size);
  html_file.close();

  // access.htmlを読み込む
  html_file = SPIFFS.open(access_path, "r");
  size = html_file.size();
  html_file.read(access_html, size);
  html_file.close();

  // setinfo.htmlを読み込む
  html_file = SPIFFS.open(setinfo_path, "r");
  size = html_file.size();
  html_file.read(setinfo_html, size);
  html_file.close();

  // urlハンドラの設定
  server.add_handler("/", handle_root);
  server.add_handler("/access.html", handle_access);
  server.add_handler("/setinfo.html", handle_setinfo);
  server.add_handler_post("/access.html", handle_access_post); 
  server.begin();
}

// GET /
void handle_root(){
  server.send_html(200, (char *)root_html);
}

// GET /access.html
void handle_access(){
  server.send_html(200, (char *)access_html);
}

// GET /setinfo.html
void handle_setinfo(){
  server.send_html(200, (char *)setinfo_html);
}

// POST /access.html
void handle_access_post(String query){
  std::map<String, String> map_query;
  int connect_try_count = 0;

  // クエリの解析
  analyze_query(query, map_query);
  Serial.print("ssid = ");
  Serial.print(map_query["ssid"]);
  Serial.print(", password = ");
  Serial.println(map_query["password"]);

  // WiFi接続処理  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(map_query["ssid"]);
  WiFi.begin(map_query["ssid"].c_str(), map_query["password"].c_str());

  while (WiFi.status() != WL_CONNECTED && connect_try_count < MAX_TRY_CONNECT) {
    delay(500);
    Serial.print(".");
    connect_try_count++;
  }

  if (WiFi.status() == WL_CONNECTED) {  
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // 接続成功したssid、passwordを覚えておく
    preferences.putString("ssid", map_query["ssid"]);
    preferences.putString("pass", map_query["password"]);
    Serial.println("Stored SSID + PASSWORD in nonvolatile storage.");
  } else {
    Serial.println("WiFi connect process time out.");
  } 
  
  server.send_html(200, (char *)success_html);
}

/*-------------------------------------------------*/
// クエリ解析処理
/*-------------------------------------------------*/
void analyze_query(String query, std::map<String, String> &map_query){
  int pos = 0;
  int index = 0;
  String q_name = "";
  while(true){
    index = query.indexOf("=", pos);
    if (index > 0) {
      q_name = query.substring(pos, index);
      //Serial.print("name = ");
      //Serial.print(q_name);
      pos = index + 1;
      index = query.indexOf("&", pos);
      if (index > 0) {
        //q_valu = query.substring(pos, index);
        map_query[q_name] = query.substring(pos, index);
        //Serial.print(", valu = ");
        //Serial.println(q_valu);
        pos = index + 1;
      } else {
        index = query.indexOf("\r", pos);
        //q_valu = query.substring(pos, index);
        map_query[q_name] = query.substring(pos, index);
        //Serial.print(", valu = ");
        //Serial.println(q_valu);
        break;
      }
    } else {
      break;
    }
  }
}

/*-------------------------------------------------*/
// 天気通知処理
/*-------------------------------------------------*/
void inform_weather(){
  //const char* host      = "numereal.sakura.ne.jp";              // 天気情報を置いてあるサーバ名
  const char* host      = "api.openweathermap.org";              // 天気情報を置いてあるサーバ名
  const char* appid     = "c99b756e06c878e45313fd8edc51a819";   // OpenWeatherMapで登録したAPPID
  const char* city      = "Nagoya";                             // 都市を選択
  const char* country   = "jp";                                 // 国を選択
  int time_number_inform_weather = 0;                           // 何時間後の天気を通知するか
  String weather[NUM_GET_WEATHER];

  Serial.print("connecting to ");
  Serial.println(host);
  
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  //String url = "/sample/esp-wroom-02/weather.php";
  // 叩くurlの設定
  String url = "/data/2.5/forecast";
  url += "?q=";
  url += city;
  url += ",";
  url += country;
  url += "&APPID=";
  url += appid;
  
  Serial.print("Requesting URL: ");
  Serial.println(url);

  // APIを叩く
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" + 
    "Connection: close\r\n\r\n");
  delay(1000);

  // 返ってきた情報の解析
  while(client.available()){
    String line = client.readStringUntil('\r');
    line.trim();
    //Serial.println(line);

    int index_last = 0;
    // NUM_GET_WEATHER分だけ文字列から天気を取り出す
    for(int i = 0; i < NUM_GET_WEATHER; i++){
      int index_weather = line.indexOf("weather", index_last);
      if(index_weather > 0){
        Serial.print("index_weather = ");
        Serial.println(index_weather);
        int index_weather_main = line.indexOf("main", index_weather);
        if(index_weather_main > 0){
          Serial.print("index_weather_main = ");
          Serial.println(index_weather_main);
          int index_weather_last = line.indexOf("\"", index_weather_main+7);
          weather[i] = line.substring(index_weather_main+7, index_weather_last);
          Serial.print("weather[");
          Serial.print(i);
          Serial.print("] = ");
          Serial.println(weather[i]);
          index_last = index_weather_main + 14;
        }
      }
    }

    if(weather[time_number_inform_weather].equals("Clouds")){
      // 曇りのときの処理
      //Serial.println("Get Cloud!");
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_BLUE, LOW);
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_WHITE, HIGH);
    }else if(weather[time_number_inform_weather].equals("Clear")){
      // 晴れの時の処理
      //Serial.println("Get Clear!");
      digitalWrite(PIN_LED_WHITE, LOW);
      digitalWrite(PIN_LED_BLUE, LOW);
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_RED, HIGH);
    }else if(weather[time_number_inform_weather].equals("Rain")){
      // 雨の時の処理
      //Serial.println("Get Rain!");
      digitalWrite(PIN_LED_WHITE, LOW);
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_BLUE, HIGH);
    }else if(weather[time_number_inform_weather].equals("Snow")){
      // 雪の時の処理
      //Serial.println("Get Snow!");
      digitalWrite(PIN_LED_WHITE, LOW);
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_BLUE, LOW);
      digitalWrite(PIN_LED_GREEN, HIGH);
    }
  }
  
  Serial.println();
  Serial.println("closing connection");
}
