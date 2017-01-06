//#######################################################################################################
//########################### Controller Plugin 009: HTTP/S Request #####################################
//#######################################################################################################

#ifdef CPLUGIN_009

#define CPLUGIN_ID_009    9
#define CPLUGIN_NAME_009  "HTTP/S Request"

struct TlsClient
{
  String            hostname;
  unsigned short    port;
  String            path;
  String            headers;
  WiFiClientSecure  tlsClient;
};

struct HTTPRequestHandler
{
  HTTPClient* pHttpClient;
  TlsClient*  pTlsClient;
};

//********************************************************************************
// CPlugin_009 : HTTP/S request manager.
//********************************************************************************
boolean CPlugin_009(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
    {
      Protocol[++protocolCount].Number = CPLUGIN_ID_009;
      Protocol[protocolCount].selectHttpMethod = true;
      Protocol[protocolCount].defineHttpUrl = true;
      Protocol[protocolCount].defineHttpHeader = true;
      Protocol[protocolCount].defineHttpBody = true;
      Protocol[protocolCount].defineTlsThumbprint = true;
      break;
    }

    case CPLUGIN_GET_DEVICENAME:
    {
      string = F(CPLUGIN_NAME_009);
      break;
    }
      
    case CPLUGIN_PROTOCOL_SEND:
    {
      HTTPRequest_SendBySensor(event);
      break;
    }
  }

  return success;
}

//********************************************************************************
// Send the request to the HTTP/S backend depending on the sensor type.
//********************************************************************************
void HTTPRequest_SendBySensor(struct EventStruct *event)
{
  switch (event->sensorType)
  {
    case SENSOR_TYPE_SINGLE:
    case SENSOR_TYPE_SWITCH:
    case SENSOR_TYPE_DIMMER:
    {
      HTTPRequest_Send(event, 0, UserVar[event->BaseVarIndex], 0);
      break;
    }
    
    case SENSOR_TYPE_LONG:
    {
      HTTPRequest_Send(event, 0, 0,
                      (unsigned long)UserVar[event->BaseVarIndex] + ((unsigned long)UserVar[event->BaseVarIndex + 1] << 16));
      break;
    }
    
    // 2 values in 1 sensor.
    case SENSOR_TYPE_TEMP_HUM:
    case SENSOR_TYPE_TEMP_BARO:
    {
      HTTPRequest_SendAndWait(event, 0, UserVar[event->BaseVarIndex], 0);
      HTTPRequest_Send(event, 1, UserVar[event->BaseVarIndex + 1], 0);
      break;
    }

    // 3 values in 1 sensor.
    case SENSOR_TYPE_TEMP_HUM_BARO:
    {
      HTTPRequest_SendAndWait(event, 0, UserVar[event->BaseVarIndex], 0);
      HTTPRequest_SendAndWait(event, 1, UserVar[event->BaseVarIndex + 1], 0);
      HTTPRequest_Send(event, 2, UserVar[event->BaseVarIndex + 2], 0);
      break;
    }
  }
}

//********************************************************************************
// Send the request to the HTTP/S backend and wait for a period of time.
//********************************************************************************
void HTTPRequest_SendAndWait(struct EventStruct *event,
                             byte varIndex,
                             float value,
                             unsigned long longValue)
{
  HTTPRequest_Send(event, varIndex, value, longValue);

  unsigned long timer = millis() + Settings.MessageDelay;
  while (millis() < timer)
  {
    backgroundtasks();
  }
}

//********************************************************************************
// Send the request to the HTTP/S backend.
//********************************************************************************
void HTTPRequest_Send(struct EventStruct *event,
                      byte varIndex,
                      float value,
                      unsigned long longValue)
{
  struct HTTPRequestHandler req;
  String payload;

  if (HTTPRequest_setupClient(&req, event, varIndex, value, longValue))
  {
    HTTPRequest_setRequestHeaders(&req, event, varIndex, value, longValue);
    HTTPRequest_buildRequestPayload(payload, event, varIndex, value, longValue);
    HTTPRequest_sendRequest(&req, payload);
  }
  else
  {
    addLog(LOG_LEVEL_DEBUG, "HTTP Request : connection failed");
  }
  HTTPRequest_destroyClient(&req);
}

//********************************************************************************
// Setup the HTTPRequestHandler object.
//********************************************************************************
boolean HTTPRequest_setupClient(struct HTTPRequestHandler* pReq,
                                struct EventStruct *event,
                                byte varIndex,
                                float value,
                                unsigned long longValue)
{
  memset(pReq, 0x00, sizeof(struct HTTPRequestHandler));

  // Endpoint.
  String h = String(Settings.HttpUrl);
  ReplaceTokenByValue(h, event, varIndex, value, longValue);
  addLog(LOG_LEVEL_DEBUG, String("Connection to ") + h);

  // With HTTP/S ?
  if (h.startsWith("https"))
  {
    String f = String(SecuritySettings.TlsThumbprint);
    if (f.length() > 0)
    {
      addLog(LOG_LEVEL_DEBUG_MORE, String("TLS Fingerprint is ") + f);
      addLog(LOG_LEVEL_DEBUG_MORE, "Creating HTTPClient with TLS");
      pReq->pHttpClient = new HTTPClient();
      return pReq->pHttpClient->begin(h, f);
    }
    else
    {
      // HTTPClient doesn't work with TLS is fingerprint isn't provided.
      // Unforetunately it switch internally to http instead of https...
      // ...so allocate a WiFiClientSecure instead !
      addLog(LOG_LEVEL_DEBUG_MORE, "HTTPS without TLS Fingerprint");
      addLog(LOG_LEVEL_DEBUG_MORE, "Creating WiFiClientSecure without TLS");

      // Need to split 'h' https://ABC.DEF.GHI[:XYZ][/PATH] to :
      // - hostname : ABC.DEF.GHI
      // - port : XYZ or 443
      // - path : /PATH
      pReq->pTlsClient = new TlsClient();
      pReq->pTlsClient->hostname = h.substring(8, h.indexOf("/", 8));
      String port = pReq->pTlsClient->hostname.substring(pReq->pTlsClient->hostname.indexOf(":"));
      pReq->pTlsClient->port = (port.length() > 0) ? port.toInt() : 443;
      pReq->pTlsClient->path = h.substring(h.indexOf("/", 8));

      return pReq->pTlsClient->tlsClient.connect(pReq->pTlsClient->hostname.c_str(), pReq->pTlsClient->port);
    }
  }
  else
  {
    addLog(LOG_LEVEL_DEBUG_MORE, "Creating HTTPClient without TLS");
    pReq->pHttpClient = new HTTPClient();
    return pReq->pHttpClient->begin(h);
  }
}

//********************************************************************************
// Clean the HTTPRequestHandler object.
//********************************************************************************
void HTTPRequest_destroyClient(struct HTTPRequestHandler* pReq)
{
  if (pReq->pHttpClient)
  {
    addLog(LOG_LEVEL_DEBUG_MORE, "Cleaning the HTTPClient object");
    delete pReq->pHttpClient;
    pReq->pHttpClient = NULL;
  }

  if (pReq->pTlsClient)
  {
    addLog(LOG_LEVEL_DEBUG_MORE, "Cleaning the TlsClient object");
    pReq->pTlsClient->tlsClient.stop();
    delete pReq->pTlsClient;
    pReq->pTlsClient = NULL;
  }
}

//********************************************************************************
// Set the headers.
//********************************************************************************
void HTTPRequest_setRequestHeaders(struct HTTPRequestHandler* pReq,
                                   struct EventStruct *event,
                                   byte varIndex,
                                   float value,
                                   unsigned long longValue)
{
  String headers = String(Settings.HttpHeader);
  
  // Check headers existence.
  if (headers.length() == 0)
  {
    addLog(LOG_LEVEL_DEBUG_MORE, "No headers set");
    return;
  }

  // Set the values.
  ReplaceTokenByValue(headers, event, varIndex, value, longValue);
  addLog(LOG_LEVEL_DEBUG_MORE, "Headers are " + headers);

  // Set the headers depending of the client.
  if (pReq->pHttpClient)
  {
    int from;
    int to = 0;
    while (to < headers.length())
    {
      from = to;
      to = headers.indexOf(":", from);

      addLog(LOG_LEVEL_DEBUG_MORE, "Extracting key from " + String(from) + " to " + String(to));
      String key = headers.substring(from, to);

      from = ++to;
      to = headers.indexOf("\r\n", from);
      if (to == -1)
        to = headers.length();

      addLog(LOG_LEVEL_DEBUG_MORE, "Extracting value from " + String(from) + " to " + String(to));
      String value = headers.substring(from, to);
      to += 2;
      value.trim();

      addLog(LOG_LEVEL_DEBUG_MORE, "Header key : " + key);
      addLog(LOG_LEVEL_DEBUG_MORE, "Header value : " + value);

      pReq->pHttpClient->addHeader(key, value);
    }
  }
  else if (pReq->pTlsClient)
  {
    pReq->pTlsClient->headers = String(Settings.HttpMethod) + " " + pReq->pTlsClient->path + " HTTP/1.1\r\n" +
                                "Host : " + pReq->pTlsClient->hostname + "\r\n" +
                                headers;
    addLog(LOG_LEVEL_DEBUG_MORE, "TLS Headers value : " + pReq->pTlsClient->headers);
  }
}

//********************************************************************************
// Build the payload string.
//********************************************************************************
void HTTPRequest_buildRequestPayload(String& payload,
                                     struct EventStruct *event,
                                     byte varIndex,
                                     float value,
                                     unsigned long longValue)
{
  payload = String(Settings.HttpBody);
  if (payload.length() > 0)
  {
    // Read the values.
    if (ExtraTaskSettings.TaskDeviceValueNames[0][0] == 0)
      PluginCall(PLUGIN_GET_DEVICEVALUENAMES, event, dummyString);
    
    ReplaceTokenByValue(payload, event, varIndex, value, longValue);
    addLog(LOG_LEVEL_DEBUG_MORE, "Payload is " + payload);
  }
}

//********************************************************************************
// Send the request according to the HTTP method.
//********************************************************************************
void HTTPRequest_sendRequest(struct HTTPRequestHandler* pReq, String& payload)
{
  String method = String(Settings.HttpMethod);
  addLog(LOG_LEVEL_DEBUG, String("Sending the ") + method + String(" request"));

  // Send the request depending on the client.
  int ret = 0;
  if (pReq->pHttpClient)
  {
    if (method.equals("POST"))
    {
      ret = pReq->pHttpClient->POST(payload);
    }
    else if (method.equals("PUT"))
    {
      // PUT seems to be not declared...
      // ret = client.PUT(payload);
      ret = pReq->pHttpClient->sendRequest("PUT", payload);
    }
    addLog(LOG_LEVEL_DEBUG, String("Result is code ") + String(ret));
  }
  else if (pReq->pTlsClient)
  {
    String payloadSz = String(payload.length());
    String request = pReq->pTlsClient->headers + "\r\n" +
                     "Content-Length: " + payloadSz + "\r\n\r\n" +
                     payload;
    ret = pReq->pTlsClient->tlsClient.print(payload.c_str());
    addLog(LOG_LEVEL_DEBUG, "Send " + String(ret) + " bytes of " + payloadSz);
  }
}

#endif
