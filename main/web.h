#ifndef ACTIVITY_WEB_H
#define ACTIVITY_WEB_H
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <stdio.h>
#include "secrets.h"
#include "logger.h"

// https server port
static const int _ACTIVITY_WEB__SERVER_PORT = 443;
// buffer size for request path (e.g "/activity/get")
static const int _ACTIVITY_WEB_PATH_BUF_SIZE = 256;
// buffer size for request body (e.g. JSON payload)
static const int _ACTIVITY_WEB_DATA_BUF_SIZE = 1024;

// Current request - only one request is allowed at one time
enum RequestState {
  IDLE, GET_TYPE, START, END
};

class ActivityWebClient {
public:
  ActivityWebClient() {
    client.setInsecure();
    http.connectionKeepAlive();
  }
  // Async
  bool getLastActivityType(int userID, void (*callback)(int));
  // Async
  bool startActivity(int userID, int type, void (*callback)(bool));
  // Async
  bool endActivity(int userID, int type, void (*callback)(bool));
  // For scheduler looping
  void looper();

private:
  WiFiClientSecure client;
  HttpClient http = HttpClient(client, ACTIVITY_SECRET_SERVER, _ACTIVITY_WEB__SERVER_PORT);
  ActivityLogger logger = ActivityLogger("WEB");

  char path[_ACTIVITY_WEB_PATH_BUF_SIZE];
  char data[_ACTIVITY_WEB_DATA_BUF_SIZE];

  static const char *startPath;
  static const char *endPath;
  static const char *lastActivityPath;
  static const char *requestBody;
  static const char *jsonContentType;

  void abandonSyncState();
  
  // async state machine states
  RequestState state = IDLE;
  int statusCode = -1;
  void (*getLastActivityTypeCallback) (int);
  void (*startActivityCallback) (bool);
  void (*endActivityCallback) (bool);
};

#endif
