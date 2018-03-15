sensor_data = [{y:0, label: 'OD'}, {y:0, label: 'Purpleness'}]
effector_data = [{y:0, label: 'Stir'}, {y:0, label: 'Air'}, {y:0, label: 'Fan'}, {y:0, label: 'Peltier'}]
temp_data = [{y:0, label: 'Temp'}]
closed_loop = true;

window.onload = function () {
  peltier_button();
    var sensor_chart = new CanvasJS.Chart("sensorContainer", {
      theme: "light2",
      axisY: {
        maximum: 1023,
        title: "Arbitrary units (0-1023)"
      },
      data: [{        
        type: "column",  
        dataPoints: sensor_data
      }]
    });
    var effector_chart = new CanvasJS.Chart("effectorContainer", {
      animationEnabled: false,
      theme: "light2",
      axisY: {
        maximum: 255,
        title: "Arbitrary units (0-255)"
      },
      data: [{        
        type: "column",  
        dataPoints: effector_data
      }]
    });
    var temp_chart = new CanvasJS.Chart("tempContainer", {
      animationEnabled: false,
      theme: "light2",
      axisY: {
        maximum: 40,
        title: "Arbitrary units (0-255)"
      },
      data: [{        
        type: "column",  
        dataPoints: temp_data
      }]
    });
    sensor_chart.render();
    effector_chart.render();
    temp_chart.render();

    var updateCharts = function () {
      sensor_chart.render();
      effector_chart.render();
      temp_chart.render();
    };
    setInterval(function(){updateCharts()}, 1000);
}

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
  var signal = 's'+document.getElementById('stir_slider').value.toString();
  console.log(signal);
  connection.send(signal);
}

function changeAirSet () {
  change_label('air_slider', 'air_set');
  var signal = 'a'+document.getElementById('air_slider').value.toString();
  console.log(signal);
  connection.send(signal);
}

function changeFanSet () {
  change_label('fan_slider', 'fan_set');
  var signal = 'f'+String(document.getElementById('fan_slider').value);
  console.log(signal);
  connection.send(signal);
}

function changeHeatSet () {
  change_label('heat_slider', 'heat_set');
  var signal = 'h'+document.getElementById('heat_slider').value.toString();
  console.log(signal);
  connection.send(signal);
}

function change_label(slider, label) {
  var val = document.getElementById(slider).value;
  document.getElementById(label).innerHTML = val;
}

function activate_closed_loop() {
  closed_loop = !closed_loop;
  var signal = 'c';
  if (closed_loop) {
    signal += '1'
  } else {
    signal += '0'
  }
  peltier_button();
  console.log(signal);
  connection.send(signal);
}

function peltier_button() {
  if (closed_loop) {
    document.getElementById('closed_loop').innerHTML = 'Turn closed-loop temperature control OFF';
    document.getElementById('heat_slider').className = 'disabled';
    document.getElementById('heat_slider').disabled = true;
  } else {
    document.getElementById('closed_loop').innerHTML = 'Turn closed-loop temperature control ON';
    document.getElementById('heat_slider').className = 'enabled';
    document.getElementById('heat_slider').disabled = false;
  }
}
