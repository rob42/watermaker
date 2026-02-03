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
#define POST_MEMBRANE_PRESSURE_PIN GPIO_NUM_3  // for high pressure post membrane
#define TDI_PIN GPIO_NUM_4  // for tdi measurement
#define MV_TO_LOW_MPA 0.0001738 //convert 2877mV  to 0.5Mpa
#define MV_TO_HIGH_MPA 0.003823 //convert 2877mV to 11Mpa
#define MPA_TO_PA 1000000 //Mpa to Pa
#define MPA_TO_PSI 145.0377
#define PSI_TO_MPA 0.006894757
#define C_TO_K 273.15 //degrees C to K
#define MV_TO_PPM 0.4347
#define RELAY_PIN 21// turn watermaker on or off
#define STARTUP_DELAY 1000*60
#define RESTART_DELAY 1000*60
#define LED_BLUE 8 // blue LED pin

#include "watermaker.h"

//setup onewire
OneWire oneWireBoostTemp(BOOST_TEMP_PIN);
OneWire oneWireHpTemp(HP_TEMP_PIN);
DallasTemperature boostSensor(&oneWireBoostTemp);
DallasTemperature hpSensor(&oneWireHpTemp);

unsigned long startTime = 0;
unsigned long stopTime = 0;
unsigned long wmLastTime = 0;
unsigned long wmTimerDelay = 2000;
//in deg Kelvin
double boostTemp = 0.0;
double hpTemp = 0.0;
//in Pa units, averaged
RunningAverage preFilterPressure(10); //output is 0.5-4.5V, voltage divider = 0.32-2.877V, so conv = (mV-320) * D_TO_LOW_MPA
RunningAverage postFilterPressure(10);
RunningAverage postMembranePressure(10); //output is 0.5-4.5V, voltage divider = 0.32-2.877V, so conv = (mV-320) * D_TO_HIGH_MPA
//tdi avaeraged
RunningAverage tdi(10); // in ppm. Sensor is 0-2.3V, 0-1000ppm, so conversion = mV *.0023
bool running = false;
bool safe = false;
// Set latest data (boost T, hp T, prefilter P, post filter P, post membrane P, tdi v )
void setData()
{

  // also expose to webserver sensor map 
  
  webServerNode.setSensorData("boostTemperature", boostTemp);
  webServerNode.setSensorData("hpTemperature", hpTemp);
  webServerNode.setSensorData("preFilterPressure", preFilterPressure.getAverage() * MPA_TO_PSI);
  webServerNode.setSensorData("postFilterPressure", postFilterPressure.getAverage() * MPA_TO_PSI);
  webServerNode.setSensorData("postMembranePressure", postMembranePressure.getAverage() * MPA_TO_PSI);
  webServerNode.setSensorData("tdi", tdi.getAverage());
  webServerNode.setSensorData("running", running);

  //setup values for zenoh
  
  readings["watermaker"]["main"]["boostPump"]["temperature"] = floor(boostTemp);
  readings["watermaker"]["main"]["highPressurePump"]["temperature"] = floor(hpTemp);
  readings["watermaker"]["main"]["preFilterPressure"] = floor(preFilterPressure.getAverage()*MPA_TO_PA);
  readings["watermaker"]["main"]["postFilterPressure"] = floor(postFilterPressure.getAverage()*MPA_TO_PA);
  readings["watermaker"]["main"]["preMembranePressure"] = floor(postFilterPressure.getAverage()*MPA_TO_PA);
  readings["watermaker"]["main"]["postMembranePressure"] = floor(postMembranePressure.getAverage()*MPA_TO_PA);
  readings["watermaker"]["main"]["productSalinity"] = floor(tdi.getAverage());
  readings["watermaker"]["main"]["runTime"] = floor((millis()-startTime)*.0001); //seconds
  if(running){
    readings["watermaker"]["main"]["status"] = "RUNNING";
    readings["watermaker"]["main"]["runTime"] = floor((millis()-startTime)*.001); //seconds
  }else{
    readings["watermaker"]["main"]["status"] = "STOPPED";
    readings["watermaker"]["main"]["runTime"] = floor((stopTime-startTime)*.001); //seconds
  }
}

void updateTemperatures(){
  syslog.information.println("Try boost temperature..");
  boostSensor.requestTemperatures();
  boostTemp= boostSensor.getTempCByIndex(0) + C_TO_K;
  syslog.information.print("boostPump Temp: ");
  syslog.information.println(boostTemp);
  
  syslog.information.println("Try hp temperature..");
  hpSensor.requestTemperatures();
  hpTemp = hpSensor.getTempCByIndex(0) + C_TO_K;
  syslog.information.print("hpTemp Temp: ");
  syslog.information.println(hpTemp);
  
}

void readPressures(){
  //signalk = Pa
  //will read a voltage(0-3.2v), which corresponds to the range of pressure
  //low pressure = 0-0.5MPa, 0-72.5 psi
  syslog.information.println("Read pressures..");
  float prefMv = analogReadMilliVolts(PRE_FILTER_PRESSURE_PIN);
  syslog.information.printf("prefMv = %f\n",prefMv);
  preFilterPressure.addValue((prefMv-320)*MV_TO_LOW_MPA) ;

  float postfMv = analogReadMilliVolts(POST_FILTER_PRESSURE_PIN);
  syslog.information.printf("postfMv = %f\n",postfMv);
  postFilterPressure.addValue((postfMv-320)*MV_TO_LOW_MPA) ;

  //high pressure will be 0-11 MPa, 0-1600psi
  float hpMv = analogReadMilliVolts(POST_MEMBRANE_PRESSURE_PIN);
  syslog.information.printf("hpMv = %f\n",hpMv);
  postMembranePressure.addValue((hpMv-340)*MV_TO_HIGH_MPA);

}

void readTdi(){
  float tdiMv = analogReadMilliVolts(TDI_PIN);
  syslog.information.printf("tdi = %f\n",tdiMv);
  tdi.addValue(tdiMv*MV_TO_PPM);
}

void runWatermaker(){
  syslog.information.print("Running watermaker: ");
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  running=true;
  startTime = millis();
  syslog.information.println(running);
 
}

void stopWatermaker(){
  syslog.information.print("Stopping watermaker: ");
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_BLUE, LOW);
  running=false;
  stopTime = millis();
  syslog.information.println(running);
  
}

/*Make sure params are within safety limits*/
void checkSafe(){
  bool isOk = true;
  //tdi < 500
  if(tdi.getAverage()>500){
    isOk = false;
    syslog.error.printf("TDI exceeded 500, actual value: %.2f \n", tdi.getAverage());
  }
  //prefilterPressure 10<pressure<30
  if(preFilterPressure.getAverage() < (10.0*PSI_TO_MPA) || preFilterPressure.getAverage() > (30.0*PSI_TO_MPA)) {
    isOk = false;
    syslog.error.printf("preFilterPressure under 10psi, or over 30psi, actual value: %.2f \n", preFilterPressure.getAverage()*PSI_TO_MPA);
  }
  //postFilterPressure > (prefilterPressure - 3)
  if(preFilterPressure.getAverage() - postFilterPressure.getAverage() > 4.0*PSI_TO_MPA) {
    isOk = false;
    syslog.error.printf("Pressure drop across filters over 4psi, actual value: %.2f \n", (preFilterPressure.getAverage() - postFilterPressure.getAverage())*PSI_TO_MPA);
  }
  // 750 < postMembranePressure <850
  if(postMembranePressure.getAverage() < (750.0*PSI_TO_MPA) || postMembranePressure.getAverage() > (850.0*PSI_TO_MPA)){
    isOk = false;
    syslog.error.printf("postMembranePressure under 750, or over 850, actual value: %.2f \n", (postMembranePressure.getAverage()*PSI_TO_MPA));
  } 
  //bostPump temp < 90C
  if(boostTemp > (90.0+C_TO_K)){
    isOk = false;
    syslog.error.printf("boostPump temerature exceeded 90C, actual value: %.2f \n", boostTemp-C_TO_K);
  } 
  //hpPump temp < 90C
  if(hpTemp > (90.0+C_TO_K)) {
    isOk = false;
    syslog.error.printf("hpPump temerature exceeded 90C, actual value: %.2f \n", hpTemp-C_TO_K);
  } 

  syslog.error.printf("Safe? : %s\n", isOk?"true":"false");
  //if we have an error set running to false.
  if(!isOk) stopWatermaker();

}

// *****************************************************************************
void setup()
{
  // Initialize base subsystems (WiFi, OTA, WebServer, Zenoh, Syslog)
  startTime = millis();
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
  runWatermaker();
}

// *****************************************************************************
void loop()
{
  // run base periodic tasks (Zenoh publish, OTA, web updates)
  baseLoopTasks();

  if ((millis() - wmLastTime) > wmTimerDelay)
    {
      wmLastTime=millis();
      //readTemperatures();
      
      //update data
      updateTemperatures();
      //read current pressures
      readPressures();

      readTdi();
      
      setData();
      
      if(running && (millis()-startTime) > STARTUP_DELAY){
        checkSafe();
      }
      if(!running && (millis()-stopTime) > RESTART_DELAY){
        runWatermaker();      
      }
    }

}

