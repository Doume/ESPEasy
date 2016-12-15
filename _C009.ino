//#######################################################################################################
//########################### Controller Plugin 009: HTTP/S Request #####################################
//#######################################################################################################


#define CPLUGIN_009
#define CPLUGIN_ID_009    9
#define CPLUGIN_NAME_009  "HTTP/S Request"


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

        // HTTP Request : full request feature.
        Protocol[protocolCount].selectHttpMethod = true;
        Protocol[protocolCount].defineHttpUrl = true;
        Protocol[protocolCount].defineHttpHeader = true;
        Protocol[protocolCount].defineHttpBody = true;

        // TLS Support.
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
        switch (event->sensorType)
        {
          case SENSOR_TYPE_SINGLE:                      // single value sensor, used for Dallas, BH1750, etc
          case SENSOR_TYPE_SWITCH:
          case SENSOR_TYPE_DIMMER:
            HTTPSendRequest(event, 0, UserVar[event->BaseVarIndex], 0);
            break;
          case SENSOR_TYPE_LONG:                      // single LONG value, stored in two floats (rfid tags)
            HTTPSendRequest(event, 0, 0, (unsigned long)UserVar[event->BaseVarIndex] + ((unsigned long)UserVar[event->BaseVarIndex + 1] << 16));
            break;
          case SENSOR_TYPE_TEMP_HUM:
          case SENSOR_TYPE_TEMP_BARO:
            {
              HTTPSendRequest(event, 0, UserVar[event->BaseVarIndex], 0);
              unsigned long timer = millis() + Settings.MessageDelay;
              while (millis() < timer)
                backgroundtasks();
              HTTPSendRequest(event, 1, UserVar[event->BaseVarIndex + 1], 0);
              break;
            }
          case SENSOR_TYPE_TEMP_HUM_BARO:
            {
              HTTPSendRequest(event, 0, UserVar[event->BaseVarIndex], 0);
              unsigned long timer = millis() + Settings.MessageDelay;
              while (millis() < timer)
                backgroundtasks();
              HTTPSendRequest(event, 1, UserVar[event->BaseVarIndex + 1], 0);
              timer = millis() + Settings.MessageDelay;
              while (millis() < timer)
                backgroundtasks();
              HTTPSendRequest(event, 2, UserVar[event->BaseVarIndex + 2], 0);
              break;
            }
        }
        break;
      }

  }
  return success;
}

//********************************************************************************
// Send the HTTP request to the backend.
//********************************************************************************
void HTTPSendRequest(struct EventStruct *event,
                     byte varIndex,
                     float value,
                     unsigned long longValue)
{
  // Setup the HTTP client & headers.
  HTTPClient client;
  setupHttpClient(client, event, varIndex, value, longValue);
  setRequestHeaders(client, event, varIndex, value, longValue);

  // Build the payload & send the request.
  String payload;
  buildRequestPayload(payload, event, varIndex, value, longValue);
  sendRequest(client, payload);
}

//********************************************************************************
// Setup the HTTPClient object.
//********************************************************************************
void setupHttpClient(HTTPClient& client,
                     struct EventStruct *event,
                     byte varIndex,
                     float value,
                     unsigned long longValue)
{
  // Endpoint.
  String h = String(Settings.HttpUrl);
  ReplaceTokenByValue(h, event, varIndex, value, longValue);

  // With TLS ?
  if (h.startsWith("https"))
  {
    String f = String(SecuritySettings.TlsThumbprint);
    addLog(LOG_LEVEL_DEBUG, String("TLS Fingerprint is ") + f);
    client.begin(h, f);
  }
  else
  {
    client.begin(h);
  }

  addLog(LOG_LEVEL_DEBUG, String("Connection to ") + h);
}

//********************************************************************************
// Set the headers.
//********************************************************************************
void setRequestHeaders(HTTPClient& client,
                       struct EventStruct *event,
                       byte varIndex,
                       float value,
                       unsigned long longValue)
{
  String header = String(Settings.HttpHeader);
  addLog(LOG_LEVEL_DEBUG_MORE, "Headers are " + header);
  if (header.length() > 0)
  {
    ReplaceTokenByValue(header, event, varIndex, value, longValue);

    int from;
    int to = 0;
    while (to < header.length())
    {
      from = to;
      to = header.indexOf(":", from);

      addLog(LOG_LEVEL_DEBUG_MORE, "Extracting key from " + String(from) + " to " + String(to));
      String key = header.substring(from, to);

      from = ++to;
      to = header.indexOf("\r\n", from);
      if (to == -1)
        to = header.length();

      addLog(LOG_LEVEL_DEBUG_MORE, "Extracting value from " + String(from) + " to " + String(to));
      String value = header.substring(from, to);
      to += 2;
      value.trim();

      addLog(LOG_LEVEL_DEBUG, "Header key : " + key);
      addLog(LOG_LEVEL_DEBUG, "Header value : " + value);

      client.addHeader(key, value);
    }
  }
}

//********************************************************************************
// Build the payload string.
//********************************************************************************
void buildRequestPayload(String& payload,
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
    addLog(LOG_LEVEL_DEBUG, "Payload is " + payload);
  }
}

//********************************************************************************
// Send the request according to the HTTP method.
//********************************************************************************
void sendRequest(HTTPClient& client, String& payload)
{
  String method = String(Settings.HttpMethod);
  addLog(LOG_LEVEL_DEBUG, String("Sending the ") + method + String(" request"));

  int sz = 0;
  if (method.equals("POST"))
  {
    sz = client.POST(payload);
  }
  else if (method.equals("PUT"))
  {
    //sz = client.PUT(payload);
    addLog(LOG_LEVEL_DEBUG, "HTTP PUT is not supported !");
  }

  // Log the result.
  addLog(LOG_LEVEL_DEBUG, String("Result is code ") + String(sz));
}


