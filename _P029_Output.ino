//#######################################################################################################
//#################################### Plugin 029: Output ###############################################
//#######################################################################################################

#ifdef PLUGIN_029

#define PLUGIN_ID_029         29
#define PLUGIN_NAME_029       "Output - (Domoticz MQTT helper)"
#define PLUGIN_VALUENAME1_029 "Output"
boolean Plugin_029(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_029;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_029);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_029));
        break;
      }

    case PLUGIN_INIT:
      {
        success = true;
        break;
      }
  }
  return success;
}

#endif