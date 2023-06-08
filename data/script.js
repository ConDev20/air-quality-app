// Create Temperature Gauge
var gaugeTemp = new LinearGauge({
  renderTo: 'gauge-temperature',
  width: 150,
  height: 450,
  units: "Температура ℃",
  minValue: 0,
  startAngle: 90,
  ticksAngle: 180,
  maxValue: 40,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueDec: 2,
  valueInt: 2,
  majorTicks: [
      "0",
      "5",
      "10",
      "15",
      "20",
      "25",
      "30",
      "35",
      "40"
  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 30,
          "to": 40,
          "color": "rgba(200, 50, 50, .75)"
      }
  ],
  colorPlate: "#fff",
  colorBarProgress: "#CC2936",
  colorBarProgressEnd: "#049faa",
  borderShadowWidth: 0,
  borders: false,
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  barWidth: 10,
}).draw();
  
// Create Pressure Gauge
var gaugePre = new RadialGauge({
  renderTo: 'gauge-pressure',
  width: 300,
  height: 300,
  units: "Налягане (hPa)",
  minValue: 700,
  maxValue: 1200,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueInt: 2,
  majorTicks: [
      "700",
      "800",
      "900",
      "1000",
      "1100",
      "1200"

  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 900,
          "to": 1100,
          "color": "#007F80"
      }
  ],
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#007474",
  colorNeedleEnd: "#800000",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#007F80",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear"
}).draw();


var Socket;

function init() {
  Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
  Socket.onmessage = function(event) {
    onMessage(event);
  };
}

function updateThreshold(id, id_value) {
  var msg = {"type": id, "value" : id_value};
  var json_data = JSON.stringify(msg);
  Socket.send(json_data);
}

var pt1 = document.getElementById('pt1');
pt1.addEventListener('change',  function() {return updateThreshold('pt1', pt1.value);});

var pt25 = document.getElementById('pt25');
pt25.addEventListener('change', function() {return updateThreshold('pt25', pt25.value);});

var pt4 = document.getElementById('pt4');
pt4.addEventListener('change', function() {return updateThreshold('pt4', pt4.value);});

var pt10 = document.getElementById('pt10');
pt10.addEventListener('change', function() {return updateThreshold('pt10', pt10.value);});


const mtMap = {
  'pm1': 'pt1',
  'pm25': 'pt25',
  'pm4': 'pt4',
  'pm10': 'pt10',
};

var typicalSize = document.getElementById('typical_size');

function setStatus(key, fValue)
{
  var measurement_element = document.getElementById(key+"-status")
  measurement_element.className = '';
  measurement_element.classList.add('ConnectionIndicator');
  var treshold_element = document.getElementById(mtMap[key]);

  if ((parseFloat(treshold_element.value) > fValue) && (parseFloat(treshold_element.value) < (parseFloat(fValue) + 1.0))) {
    measurement_element.classList.add('ConnectionIndicator--trying');
  } else if (parseFloat(treshold_element.value) > fValue) {
    measurement_element.classList.add('ConnectionIndicator--connected');
  } else {
    measurement_element.classList.add('ConnectionIndicator--disconnected');
  }
  var element = document.getElementById(key);
  element.innerHTML = fValue + " µg/m3";
}

function setRelayStatus(key, status)
{
  var relay_element = document.getElementById(key+"-status");
  relay_element.className = '';
  relay_element.classList.add('ConnectionIndicator');
  if (status == 1)
  {
    var element = document.getElementById("sys_status");
    element.innerHTML = "Вентилационна система: Отворена";
    relay_element.classList.add('ConnectionIndicator--connected');
  } else {
    var element = document.getElementById("sys_status");
    element.innerHTML = "Вентилационна система: Затворена";
    relay_element.classList.add('ConnectionIndicator--disconnected');
  }
}

function onMessage(event) {
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
      var key = keys[i];
      var fValue = parseFloat(myObj[key]).toFixed(1);
      if (key in mtMap)
      {
        setStatus(key, fValue);
      }

      if (key === "pt1"){
        pt1.value = parseFloat(myObj[key]).toFixed(1);
      } else if (key == "pt25"){
        pt25.value = parseFloat(myObj[key]).toFixed(1);
      } else if (key === "pt4"){
        pt4.value = parseFloat(myObj[key]).toFixed(1);
      } else if (key == "pt10"){
        pt10.value = parseFloat(myObj[key]).toFixed(1);
      }
  
      if (key === "pressure") {
        gaugePre.value = myObj[key];
      } else if ( key === "temperature") {
        gaugeTemp.value = myObj[key] - 2;
      } else if(key === "relay") {
        setRelayStatus(key,parseInt(myObj[key]));
      } else if (key === "typical_size") {
        typicalSize.innerHTML = "Среден размер на частици: " +  parseFloat(myObj[key]).toFixed(3) + "μm";
      }
	}
}

const inputs = document.getElementsByTagName('input');
for (let i = 0; i < inputs.length; i++) {
    inputs[i].addEventListener('keypress', function (event) {
        if (event.key === 'Enter' || event.key === 'Go') {
            event.preventDefault();
            event.target.blur();
            const form = event.target.form;
            if (form) {
                form.submit();
            }
        }
    });
}

window.onload = function(event) {
  init();
}
