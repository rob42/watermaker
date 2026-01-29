/***********************************************************************/ /**
   \file   watermaker.ino
   \brief  Management of watermaker


   
   This base file provides watermaker management and exposes it over wifi/html
 */

#define NODENAME "watermaker"
#define ESP32_RELAY_PIN GPIO_NUM_21// turn watermaker on or off
#define ESP32_BOOST_TEMP_PIN GPIO_NUM_20 // one-wire temp sensor for boost pump
#define ESP32_HP_TEMP_PIN GPIO_NUM_10 // one-wire temp sensor for hp pump
#define ESP32_PRE_FILTER_PRESSURE_PIN GPIO_NUM_0  // for low pressure pre filter
#define ESP32_POST_FILTER_PRESSURE_PIN GPIO_NUM_1  // for low pressure post filter
#define ESP32_POST_MEMBRANE_PRESSURE_PIN GPIO_NUM_3  // for low pressure post filter
#define ESP32_TDI_PIN GPIO_NUM_4  // for tdi measurement

#define ESP32_RELAY_PIN 21// turn watermaker on or off

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
bool running = false;
// Set latest data (boost T, hp T, prefilter P, post filter P, post membrane P, tdi v )
void setData()
{

  // also expose to webserver sensor map 
  
  webServerNode.setSensorData("boostTemp", boostTemp);
  webServerNode.setSensorData("hpPressure", hpTemp);
  webServerNode.setSensorData("preFilterPressure", preFilterPressure);
  webServerNode.setSensorData("postFilterPressure", postFilterPressure);
  webServerNode.setSensorData("postMembranePressure", postMembranePressure);
  webServerNode.setSensorData("tdi", tdi);
  webServerNode.setSensorData("running", running);

  //setup values for zenoh
  baseReadings["watermaker"]["main"]["boostPump"]["temperature"] = boostTemp;
  baseReadings["watermaker"]["main"]["highPressurePump"]["temperature"] = hpTemp;
  baseReadings["watermaker"]["main"]["preFilterPressure"] = preFilterPressure;
  baseReadings["watermaker"]["main"]["postFilterPressure"] = postFilterPressure;
  baseReadings["watermaker"]["main"]["preMembranePressure"] = postFilterPressure;
  baseReadings["watermaker"]["main"]["postMembranePressure"] = postMembranePressure;
  baseReadings["watermaker"]["main"]["productSalinity"] = tdi;
  if(running){
    baseReadings["watermaker"]["main"]["status"] = "RUNNING";
  }else{
    baseReadings["watermaker"]["main"]["status"] = "STOPPED";
  }
}

void readTemperatures(){
  //try to read async
  syslog.information.println("read temperatures (async) ");
  boostSensor.requestTemperaturesByIndex(0);
  hpSensor.requestTemperaturesByIndex(0);
}

void runWatermaker(){
  syslog.information.print("Running watermaker: ");
  digitalWrite(ESP32_RELAY_PIN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  running=true;
  syslog.information.println(running);
}
void stopWatermaker(){
  syslog.information.print("Running watermaker: ");
  digitalWrite(ESP32_RELAY_PIN, LOW);
  digitalWrite(LED_BLUE, LOW);
  running=false;
  syslog.information.println(running);
}

// *****************************************************************************
void setup()
{
  // Initialize base subsystems (WiFi, OTA, WebServer, Zenoh, Syslog)
  ArduinoOTA.setHostname(NODENAME);
  syslog.app=NODENAME;
  baseInit();

  pinMode(LED_BLUE,OUTPUT);
  //setup relay
  pinMode(ESP32_RELAY_PIN, OUTPUT);

  //setup temperature
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
      setData();

      if(running){
        stopWatermaker();
      }else{
        runWatermaker();      
      }
    }

}

