// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);
  

var labelAws = document.getElementById('label-aws');

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var running = myObj.running;
      var startTime = myObj.startTime;
      var stopTime = myObj.stopTime;
      var productSalinity = myObj.tdi;
      var boostTemperature = myObj.boostTemperature;
      var hpTemperature = myObj.hpTemperature;
      var preFilterPressure = myObj.preFilterPressure;
      var postFilterPressure = myObj.postFilterPressure;
      var preMembranePressure = myObj.preMembranePressure;
      var postMembranePressure = myObj.postMembranePressure;

      labelRunning.textContent = running?'Yes':'No';
      labelStartTime.textContent = new Date(startTime*1000).toLocaleString();
      if(stopTime>0){
       labelStopTime.textContent = new Date(stopTime*1000).toLocaleString();
      }else{
        labelStopTime.textContent = '-';
      }
      labelProductSalinity.textContent = productSalinity.toFixed(0);
      labelBoostTemperature.textContent = (boostTemperature-273.15).toFixed(0);
      labelHpTemperature.textContent = (hpTemperature-273.15).toFixed(0);
      labelPreFilterPressure.textContent = preFilterPressure.toFixed(0);
      labelPostFilterPressure.textContent = postFilterPressure.toFixed(0);
      labelPreMembranePressure.textContent = preMembranePressure.toFixed(0);
      labelPostMembranePressure.textContent = postMembranePressure.toFixed(0);
      
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
      var running = myObj.running;
      var startTime = myObj.startTime;
      var stopTime = myObj.stopTime;
      var productSalinity = myObj.tdi;
      var boostTemperature = myObj.boostTemperature;
      var hpTemperature = myObj.hpTemperature;
      var preFilterPressure = myObj.preFilterPressure;
      var postFilterPressure = myObj.postFilterPressure;
      var preMembranePressure = myObj.preMembranePressure;
      var postMembranePressure = myObj.postMembranePressure;

      labelRunning.textContent = running?'Yes':'No';
      labelStartTime.textContent = new Date(startTime*1000).toLocaleString();
      if(stopTime>0){
       labelStopTime.textContent = new Date(stopTime*1000).toLocaleString();
      }else{
        labelStopTime.textContent = '-';
      }
      labelProductSalinity.textContent = productSalinity.toFixed(0);
      labelBoostTemperature.textContent = (boostTemperature-273.15).toFixed(0);
      labelHpTemperature.textContent = (hpTemperature-273.15).toFixed(0);
      labelPreFilterPressure.textContent = preFilterPressure.toFixed(0);
      labelPostFilterPressure.textContent = postFilterPressure.toFixed(0);
      labelPreMembranePressure.textContent = preMembranePressure.toFixed(0);
      labelPostMembranePressure.textContent = postMembranePressure.toFixed(0);
    
  }, false);
}