/***********************************************************************/ /**
   \file   watermaker.ino
   \brief  Management of watermaker


   
   This base file provides watermaker management and exposes it over wifi/html
 */

#define NODENAME "watermaker"
#define RELAY_PIN GPIO_NUM_5// turn watermaker on or off
#define BOOST_TEMP_PIN GPIO_NUM_6 // one-wire temp sensor for boost pump
#define HP_TEMP_PIN GPIO_NUM_7 // one-wire temp sensor for hp pump
#define PRE_FILTER_PRESSURE_PIN GPIO_NUM_0  // for low pressure pre filter
#define POST_FILTER_PRESSURE_PIN GPIO_NUM_1  // for low pressure post filter
#define POST_MEMBRANE_PRESSURE_PIN GPIO_NUM_3  // for low pressure post filter
#define TDI_PIN GPIO_NUM_4  // for tdi measurement
#define D_TO_LOW_MPA 0.000122 //convert 4096  to 0.5Mpa
#define D_TO_HIGH_MPA 0.0026855 //convert 4096 to 11Mpa
#define MPA_TO_PA 1000000 //Mpa to Pa
#define PA_TO_PSI 0.000145
#define C_TO_K 273.15 //degrees C to K

#define RELAY_PIN 21// turn watermaker on or off

#define LED_BLUE 8 // blue LED pin

#include "watermaker.h"

//setup onewire
OneWire oneWireBoostTemp(BOOST_TEMP_PIN);
OneWire oneWireHpTemp(HP_TEMP_PIN);
DallasTemperature boostSensor(&oneWireBoostTemp);
DallasTemperature hpSensor(&oneWireHpTemp);

unsigned long wmLastTime = 0;
unsigned long wmTimerDelay = 2000;
//in deg Kelvin
double boostTemp = 0.0;
double hpTemp = 0.0;
//in Pa units
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
  webServerNode.setSensorData("preFilterPressure", preFilterPressure * PA_TO_PSI);
  webServerNode.setSensorData("postFilterPressure", postFilterPressure * PA_TO_PSI);
  webServerNode.setSensorData("postMembranePressure", postMembranePressure * PA_TO_PSI);
  webServerNode.setSensorData("tdi", tdi);
  webServerNode.setSensorData("running", running);

  //setup values for zenoh
  
  readings["watermaker"]["main"]["boostPump"]["temperature"] = boostTemp;
  readings["watermaker"]["main"]["highPressurePump"]["temperature"] = hpTemp;
  readings["watermaker"]["main"]["preFilterPressure"] = preFilterPressure;
  readings["watermaker"]["main"]["postFilterPressure"] = postFilterPressure;
  readings["watermaker"]["main"]["preMembranePressure"] = postFilterPressure;
  readings["watermaker"]["main"]["postMembranePressure"] = postMembranePressure;
  readings["watermaker"]["main"]["productSalinity"] = tdi;
  if(running){
    readings["watermaker"]["main"]["status"] = "RUNNING";
  }else{
    readings["watermaker"]["main"]["status"] = "STOPPED";
  }
}

void readTemperatures(){
  //try to read async
  syslog.information.println("read temperatures (async) ");
  //if(boostSensor.isConversionComplete()) 
  boostSensor.requestTemperaturesByIndex(0);
  //if(boostSensor.isConversionComplete()) 
  hpSensor.requestTemperaturesByIndex(0);
  delay(750);
}

void updateTemperatures(){
  syslog.information.println("Try boost temperature..");
//  if(boostSensor.isConversionComplete()){
    boostTemp= boostSensor.getTempCByIndex(0) + C_TO_K;
    syslog.information.print("boostPump Temp: ");
    syslog.information.println(boostTemp);
//  }
  syslog.information.println("Try hp temperature..");
//  if(boostSensor.isConversionComplete()){
    hpTemp = hpSensor.getTempCByIndex(0) + C_TO_K;
    syslog.information.print("hpTemp Temp: ");
    syslog.information.println(hpTemp);
//  }
}
void runWatermaker(){
  syslog.information.print("Running watermaker: ");
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  running=true;
  syslog.information.println(running);
}

void readPressures(){
  //signalk = Pa
  //will read a voltage(0-3.2v), which corresponds to the range of pressure
  //low pressure = 0-0.5MPa, 0-72.5 psi
  syslog.information.println("Read pressures..");
  preFilterPressure = analogReadMilliVolts(PRE_FILTER_PRESSURE_PIN) ;//* D_TO_LOW_MPA * MPA_TO_PA;
  postFilterPressure = analogReadMilliVolts(POST_FILTER_PRESSURE_PIN) ;//* D_TO_LOW_MPA * MPA_TO_PA;

  //high pressure will be 0-11 MPa, 0-1600psi
  postMembranePressure = analogReadMilliVolts(POST_MEMBRANE_PRESSURE_PIN) ;//* D_TO_HIGH_MPA * MPA_TO_PA;

}

void readTdi(){
  tdi = analogRead(TDI_PIN);
}
void stopWatermaker(){
  syslog.information.print("Running watermaker: ");
  digitalWrite(RELAY_PIN, LOW);
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
  pinMode(RELAY_PIN, OUTPUT);

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
      updateTemperatures();
      //read current pressures
      readPressures();

      readTdi();
      
      setData();

      if(running){
        stopWatermaker();
      }else{
        runWatermaker();      
      }
    }

}

