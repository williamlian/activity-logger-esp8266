#include <stdio.h>
#include <stdlib.h>
#include <Arduino_JSON.h>

#include "web.h"
#include "logger.h"

const char* ActivityWebClient::startPath = "/activity/start";
const char* ActivityWebClient::endPath = "/activity/end";
const char* ActivityWebClient::lastActivityPath = "/activity/get?type=last&userID=%d";
const char* ActivityWebClient::requestBody = "{\"UserID\":%d,\"TypeID\":%d}";
const char* ActivityWebClient::jsonContentType = "text/json";

bool ActivityWebClient::getLastActivityType(int userID, void (*callback)(int)) {
  if(state != RequestState::IDLE) {
    logger.log("client busy, request ignored");
    return false;
  }
  snprintf(path, _ACTIVITY_WEB_PATH_BUF_SIZE, lastActivityPath, userID);
  if(int code = http.get(path)) {
    logger.log("failed to send request for last activity, error code %d", code);
    return false;
  };
  state = RequestState::GET_TYPE;
  getLastActivityTypeCallback = callback;
  return true;
}

bool ActivityWebClient::startActivity(int userID, int type, void (*callback)(bool)) {
  abandonSyncState();
  if(state != RequestState::IDLE) {
    logger.log("client busy, request ignored");
    return false;
  }
  snprintf(data, _ACTIVITY_WEB_DATA_BUF_SIZE, requestBody, userID, type);
  logger.log("start activity request");
  if(int code = http.post(startPath, jsonContentType, data)) {
    logger.log("failed to send request for start, error code %d", code);
    return false;
  }
  state = RequestState::START;
  startActivityCallback = callback;
  return true;
}

bool ActivityWebClient::endActivity(int userID, int type, void (*callback)(bool)) {
  abandonSyncState();
  if(state != RequestState::IDLE) {
    logger.log("client busy, request ignored");
    return false;
  }
  snprintf(data, _ACTIVITY_WEB_DATA_BUF_SIZE, requestBody, userID, type);
  if(int code = http.post(endPath, jsonContentType, data)) {
    logger.log("failed to send request for end, error code %d", code);
    return false;
  }
  state = RequestState::END;
  endActivityCallback = callback;
  return true;
}

void ActivityWebClient::abandonSyncState() {
  if(state == RequestState::GET_TYPE) {
    logger.log("abandon sync state for activity change");
    http.flush();
    http.stop();
    state = RequestState::IDLE;
    getLastActivityTypeCallback = NULL;
  }
}

void ActivityWebClient::looper() {
  if(state == RequestState::IDLE) {
    return;
  }
  if(!http.available()) {
    return;
  }
  statusCode = http.responseStatusCode();
  if(!http.headerAvailable()) {
    return;
  }
  String response = http.responseBody();
  if(statusCode != 200) {
    logger.log("bad resposne from request %s: [%d] %s", state, statusCode, response.c_str());
  }
  switch(state) {
    case RequestState::GET_TYPE: {
        int activityType;
        if(statusCode == 200) {
          JSONVar jsonResponse = JSON.parse(response);
          activityType = (int)jsonResponse["Type"]["ID"];
          logger.log("last activity %d ended at: %d", activityType, (int)jsonResponse["EndedAt"]);
          if(!(jsonResponse["EndedAt"] == null)) {
            activityType = -1;
          }
        }
        getLastActivityTypeCallback(activityType);
      }
      break;
    case RequestState::START:
      logger.log("resposne from start: [%d] %s", statusCode, response.c_str());
      startActivityCallback(statusCode == 200);
      break;
    case RequestState::END:
      logger.log("resposne from end: [%d] %s", statusCode, response.c_str());
      endActivityCallback(statusCode == 200);
      break;
  }
  state = RequestState::IDLE;
  statusCode = -1;
  getLastActivityTypeCallback = NULL;
  startActivityCallback = NULL;
  endActivityCallback = NULL;
}
