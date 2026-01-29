/***********************************************************************/ /**
   \file   watermaker.ino
   \brief  Management of watermaker


   
   This base file provides watermaker management and exposes it over wifi/html
 */

#define NODENAME "watermaker"
#define ESP32_BOOST_TEMP_PIN GPIO_NUM_21 // one-wire temp sensor for boost pump
#define ESP32_HP_TEMP_PIN GPIO_NUM_20 // one-wire temp sensor for hp pump
#define ESP32_PRE_FILTER_PRESSURE_PIN GPIO_NUM_0  // for low pressure pre filter
#define ESP32_POST_FILTER_PRESSURE_PIN GPIO_NUM_1  // for low pressure post filter
#define ESP32_POST_MEMBRANE_PRESSURE_PIN GPIO_NUM_3  // for low pressure post filter
#define ESP32_TDI_PIN GPIO_NUM_4  // for tdi measurement

#define LED_BLUE 8 // blue LED pin

#include "watermaker.h"

//setup onewire
OneWire oneWireBoostTemp(ESP32_BOOST_TEMP_PIN);
OneWire oneWireHpTemp(ESP32_BOOST_TEMP_PIN);
DallasTemperature boostSensor(&oneWireBoostTemp);
DallasTemperature hpSensor(&oneWireHpTemp);

unsigned long wmLastTime = 0;
unsigned long wmTimerDelay = 1000;
double boostTemp = 0.0;
double hpTemp = 0.0;
double preFilterPressure = 0.0;
double postFilterPressure = 0.0;
double postMembranePressure = 0.0;
double tdi = 0.0;

// Set latest data (boost T, hp T, prefilter P, post filter P, post membrane P, tdi v )
void SetData()
{

  // also expose to webserver sensor map 
  
  webServerNode.setSensorData("boostTemp", boostTemp);
  webServerNode.setSensorData("hpPressure", hpTemp);
  webServerNode.setSensorData("preFilterPressure", preFilterPressure);
  webServerNode.setSensorData("postFilterPressure", postFilterPressure);
  webServerNode.setSensorData("postMembranePressure", postMembranePressure);
  webServerNode.setSensorData("tdi", tdi);

  //setup values for zenoh
  readings["watermaker"]["main"]["boostPump"]["temperature"] = boostTemp;
  readings["watermaker"]["main"]["highPressurePump"]["temperature"] = hpTemp;
  readings["watermaker"]["main"]["preFilterPressure"] = preFilterPressure;
  readings["watermaker"]["main"]["postFilterPressure"] = postFilterPressure;
  readings["watermaker"]["main"]["preMembranePressure"] = postFilterPressure;
  readings["watermaker"]["main"]["postMembranePressure"] = postMembranePressure;
  readings["watermaker"]["main"]["productSalinity"] = tdi;
}

void readTemperatures(){
  //try to read async
  syslog.information.println("read temperatures (async) ");
  boostSensor.requestTemperaturesByIndex(0);
  hpSensor.requestTemperaturesByIndex(0);
}

// *****************************************************************************
void setup()
{
  // Initialize base subsystems (WiFi, OTA, WebServer, Zenoh, Syslog)
  ArduinoOTA.setHostname(NODENAME);
  syslog.app=NODENAME;
  baseInit();
  syslog.information.println("Configure temperature sensors..");
  boostSensor.begin();
  boostSensor.setWaitForConversion(false);
  hpSensor.begin();
  hpSensor.setWaitForConversion(false);
}

// *****************************************************************************
void loop()
{
  // run base periodic tasks (Zenoh publish, OTA, web updates)
  baseLoopTasks();

  if ((millis() - wmLastTime) > wmTimerDelay)
    {
      wmLastTime=millis();
      readTemperatures();
    
      //update data
      syslog.information.println("Try boost temperature..");
      if(boostSensor.isConversionComplete()){
        boostTemp= boostSensor.getTempCByIndex(0);
        syslog.information.print("boostPump Temp: ");
        syslog.information.println(boostTemp);
      }
      syslog.information.println("Try hp temperature..");
      if(boostSensor.isConversionComplete()){
        hpTemp= hpSensor.getTempCByIndex(0);
        syslog.information.print("hpTemp Temp: ");
        syslog.information.println(hpTemp);
      }
  }

}

