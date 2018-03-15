sensor_data = [{y:0, label: 'OD'}, {y:0, label: 'Purpleness'}]
effector_data = [{y:0, label: 'Stir'}, {y:0, label: 'Air'}, {y:0, label: 'Fan'}, {y:0, label: 'Peltier'}]
temp_data = [{y:0, label: 'Temp'}]
closed_loop = false;

window.onload = function () {

    var sensor_chart = new CanvasJS.Chart("sensorContainer", {
      animationEnabled: true,
      theme: "light2", // "light1", "light2", "dark1", "dark2"
      // title:{
      //   text: "Set points"
      // },
      axisY: {
        title: "Arbitrary units (0-1023)"
      },
      data: [{        
        type: "column",  
        // showInLegend: false, 
        // legendMarkerColor: "grey",
        // legendText: "MMbbl = one million barrels",
        dataPoints: sensor_data
      }]
    });
    var effector_chart = new CanvasJS.Chart("effectorContainer", {
      animationEnabled: true,
      theme: "light2", // "light1", "light2", "dark1", "dark2"
      // title:{
      //   text: "Set points"
      // },
      axisY: {
        title: "Arbitrary units (0-255)"
      },
      data: [{        
        type: "column",  
        // showInLegend: false, 
        // legendMarkerColor: "grey",
        // legendText: "MMbbl = one million barrels",
        dataPoints: effector_data
      }]
    });
    var temp_chart = new CanvasJS.Chart("tempContainer", {
      animationEnabled: true,
      theme: "light2", // "light1", "light2", "dark1", "dark2"
      // title:{
      //   text: "Set points"
      // },
      axisY: {
        title: "Arbitrary units (0-255)"
      },
      data: [{        
        type: "column",  
        // showInLegend: false, 
        // legendMarkerColor: "grey",
        // legendText: "MMbbl = one million barrels",
        dataPoints: temp_data
      }]
    });
    sensor_chart.render();
    effector_chart.render();
    temp_chart.render();
}


var rainbowEnable = false;
var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);
connection.onopen = function () {
  connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {
  console.log('Server: ', e.data);
  document.getElementById('dat_json').innerHTML = e.data;
  // var msg = JSON.parse(e.data);
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
};

function changeStirSet () {
  change_label('stir_slider', 'stir_set');
  var signal = 's'+document.getElementById('stir_slider').toString();
  console.log(signal);
  connection.send(signal);
}

function changeAirSet () {
  change_label('air_slider', 'air_set');
  var signal = 'a'+document.getElementById('air_slider').toString();
  console.log(signal);
  connection.send(signal);
}

function changeFanSet () {
  change_label('fan_slider', 'fan_set');
  var signal = 'f'+document.getElementById('fan_slider').toString();
  console.log(signal);
  connection.send(signal);
}

function changeHeatSet () {
  change_label('heat_slider', 'heat_set');
  var signal = 'h'+document.getElementById('heat_slider').toString();
  console.log(signal);
  connection.send(signal);
}

function change_label(slider, label) {
  var val = document.getElementById(slider).value;
  document.getElementById(label).innerHTML = val;
}

function activate_closed_loop() {
  closed_loop = !closed_loop;
  peltier_button();
}
function peltier_button() {
  if (closed_loop) {
    document.getElementById('heat_slider').className = 'disabled';
    document.getElementById('heat_slider').disabled = true;
  } else {
    document.getElementById('heat_slider').className = 'enabled';
    document.getElementById('heat_slider').disabled = false;
  }
}
peltier_button();

// function rainbowEffect () {
//   rainbowEnable = ! rainbowEnable;
//   if (rainbowEnable) {
//     connection.send("R");
//     document.getElementById('rainbow').style.backgroundColor = '#00878F';
//     document.getElementById('r').className = 'disabled';
//     document.getElementById('g').className = 'disabled';
//     document.getElementById('b').className = 'disabled';
//     document.getElementById('r').disabled = true;
//     document.getElementById('g').disabled = true;
//     document.getElementById('b').disabled = true;
//   } else {
//     connection.send("N");
//     document.getElementById('rainbow').style.backgroundColor = '#999';
//     document.getElementById('r').className = 'enabled';
//     document.getElementById('g').className = 'enabled';
//     document.getElementById('b').className = 'enabled';
//     document.getElementById('r').disabled = false;
//     document.getElementById('g').disabled = false;
//     document.getElementById('b').disabled = false;
//     sendRGB();
//   }
// }
