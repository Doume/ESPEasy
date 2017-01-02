//#######################################################################################################
//########################### Controller Plugin 010: Azure IoT Hub ######################################
//#######################################################################################################

#include <AzureIoTHub.h>
#include <AzureIoTUtility.h>
#include <AzureIoTProtocol_HTTP.h>

#define CPLUGIN_ID_010    10
#define CPLUGIN_NAME_010  "Azure IoT Hub"

//********************************************************************************
// CPlugin_010 : Azure IoT Hub request manager.
//********************************************************************************
boolean CPlugin_010(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
    {
      Protocol[++protocolCount].Number = CPLUGIN_ID_010;
      Protocol[protocolCount].defineConnectionString = true;
      Protocol[protocolCount].defineHttpBody = true; 
      break;
    }

    case CPLUGIN_GET_DEVICENAME:
    {
      string = F(CPLUGIN_NAME_010);
      break;
    }
      
    case CPLUGIN_PROTOCOL_SEND:
    {
      AzureIoTHub_SendBySensor(event);
      break;
    }
  }

  return true;
}

//********************************************************************************
// Send the request to Azure IoT Hub depending on the sensor type.
//********************************************************************************
void AzureIoTHub_SendBySensor(struct EventStruct *event)
{
  switch (event->sensorType)
  {
    case SENSOR_TYPE_SINGLE:
    case SENSOR_TYPE_SWITCH:
    case SENSOR_TYPE_DIMMER:
    {
      AzureIoTHub_Send(event, 0, UserVar[event->BaseVarIndex], 0);
      break;
    }
    
    case SENSOR_TYPE_LONG:
    {
      AzureIoTHub_Send(event, 0, 0,
                      (unsigned long)UserVar[event->BaseVarIndex] + ((unsigned long)UserVar[event->BaseVarIndex + 1] << 16));
      break;
    }
    
    // 2 values in 1 sensor.
    case SENSOR_TYPE_TEMP_HUM:
    case SENSOR_TYPE_TEMP_BARO:
    {
      AzureIoTHub_SendAndWait(event, 0, UserVar[event->BaseVarIndex], 0);
      AzureIoTHub_Send(event, 1, UserVar[event->BaseVarIndex + 1], 0);
      break;
    }

    // 3 values in 1 sensor.
    case SENSOR_TYPE_TEMP_HUM_BARO:
    {
      AzureIoTHub_SendAndWait(event, 0, UserVar[event->BaseVarIndex], 0);
      AzureIoTHub_SendAndWait(event, 1, UserVar[event->BaseVarIndex + 1], 0);
      AzureIoTHub_Send(event, 2, UserVar[event->BaseVarIndex + 2], 0);
      break;
    }
  }
}

//********************************************************************************
// Send the request to Azure IoT Hub and wait for a period of time.
//********************************************************************************
void AzureIoTHub_SendAndWait(struct EventStruct *event,
                             byte varIndex,
                             float value,
                             unsigned long longValue)
{
  AzureIoTHub_Send(event, varIndex, value, longValue);

  unsigned long timer = millis() + Settings.MessageDelay;
  while (millis() < timer)
  {
    backgroundtasks();
  }
}

//********************************************************************************
// Send the request callback.
//********************************************************************************
void AzureIoTHub_Send_Callback(/*IOTHUB_CLIENT_CONFIRMATION_RESULT*/int result, void* userContextCallback)
{
  /*
    addLog(LOG_LEVEL_DEBUG, "Azure IoT Hub : Callback called with result " + 
                            String(ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result)));
                            */
}

//********************************************************************************
// Send the request to Azure IoT Hub.
//********************************************************************************
void AzureIoTHub_Send(struct EventStruct *event,
                      byte varIndex,
                      float value,
                      unsigned long longValue)
{
  // Build the payload.
  String payload = String(Settings.HttpBody);
  if (payload.length() > 0)
  {
    // Read the values.
    if (ExtraTaskSettings.TaskDeviceValueNames[0][0] == 0)
      PluginCall(PLUGIN_GET_DEVICEVALUENAMES, event, dummyString);
    
    ReplaceTokenByValue(payload, event, varIndex, value, longValue);
    addLog(LOG_LEVEL_DEBUG, "Payload is " + payload);
  }

  // Setup Azure IoT Hub client.
  IOTHUB_CLIENT_LL_HANDLE client = IoTHubClient_LL_CreateFromConnectionString(Settings.ConnectionString, HTTP_Protocol);
  if (client == NULL)
  {
    addLog(LOG_LEVEL_DEBUG, "Azure IoT Hub : Client is null");
    return;
  }

  // Build the message.
  IOTHUB_MESSAGE_HANDLE msg = IoTHubMessage_CreateFromString(payload.c_str());
  boolean prereqOk = false;
  if (msg == NULL)
  {
    addLog(LOG_LEVEL_DEBUG, "Azure IoT Hub : Message is null");
  }
  else
  {
    prereqOk = true;
  }

  // Send the request
  if (prereqOk)
  {
    IOTHUB_CLIENT_RESULT res = IoTHubClient_LL_SendEventAsync(client, msg, (IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK)AzureIoTHub_Send_Callback, (void*)1);
    if (res == IOTHUB_CLIENT_OK)
    {
      addLog(LOG_LEVEL_DEBUG, "Azure IoT Hub : Message sent");
    }
    else
    {
      addLog(LOG_LEVEL_DEBUG, "Azure IoT Hub : Message NOT sent : " + String(res));
    }
  }

  // Free the resources.
  if (msg != NULL)
  {
    IoTHubMessage_Destroy(msg);
  }
  if (client != NULL)
  {
    IoTHubClient_LL_Destroy(client);
  }
}