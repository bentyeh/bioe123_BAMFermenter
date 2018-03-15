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

function changeSet () {
  change_label('stir_slider', 'stir_set');
  change_label('air_slider', 'air_set');
  change_label('fan_slider', 'fan_set');
  change_label('heat_slider', 'heat_set');
  
  // var r = document.getElementById('r').value** 2 / 1023;
  // var g = document.getElementById('g').value** 2 / 1023;
  // var b = document.getElementById('b').value** 2 / 1023;

  // var rgb = r << 20 | g << 10 | b;
  // var rgbstr = '#' + rgb.toString(16);
  // console.log('RGB: ' + rgbstr);
  // connection.send(rgbstr);
}

function change_label(slider, label) {
  var val = document.getElementById(slider).value;
  document.getElementById(label).innerHTML = val;
}

function activate_closed_loop() {
  document.getElementById('heat_slider').className = 'disabled';
}

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
