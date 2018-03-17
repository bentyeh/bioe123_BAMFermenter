var closed_loop_temp_ctrl = true;
var system_active = true;

var sensor = [{"y":0, "label": 'OD'}, {"y":0, "label": 'Purpleness'}];
var temp = [{"y":0, "label": 'Temp (C)'}];
var output = [{"y":0, "label": 'heat'}, {"y":0, "label": 'stir'}, {"y":0, "label": 'air'}, {"y":0, "label": 'fan'}];

// open websocket to port established by Arduino WebsocketServer
var connection = new WebSocket('ws://' + location.hostname + ':81');

connection.onopen = function () {
  connection.send('Connect ' + new Date());
};

connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};

connection.onmessage = function (e) {
  console.log('Server: ', e.data);

  // write out data to HTML
  document.getElementById('dat_json').innerHTML = "Raw Data: " + e.data;

  // parse data
  var msg = JSON.parse(e.data);

  // update closed loop heating button
  if (msg["closed_loop_temp_ctrl"] == 1) {
    closed_loop_temp_ctrl = true;
  } else if (msg["closed_loop_temp_ctrl"] == 0) {
    closed_loop_temp_ctrl = false;
  }
  closed_loop_button();

  // update pause / resume button
  if (msg["system_active"] == 1) {
    system_active = true;
  } else if (msg["system_active"] == 0) {
    system_active = false;
  }
  pause_button();

  // store data into global variables
  sensor[0].y = msg.density;
  sensor[1].y = msg.purpleness;
  temp[0].y = msg["temp(C)"];
  output[0].y = msg.heat_set;
  output[1].y = msg.stir_set;
  output[2].y = msg.air_set;
  output[3].y = msg.fan_set;

  // graph data
  draw(sensor, "#sensor", 1023);
  draw(temp, "#temp", 45);
  draw(output, "#output", 255);
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
  closed_loop_temp_ctrl = !closed_loop_temp_ctrl;
  var command = 'c';
  if (closed_loop_temp_ctrl) {
    command += '1'
  } else {
    command += '0'
  }
  console.log(command);
  connection.send(command);
  closed_loop_button();
}

function closed_loop_button() {
  if (closed_loop_temp_ctrl) {
    document.getElementById('closed_loop').innerHTML = 'Turn closed-loop temperature control OFF';
    document.getElementById('heat_slider').className = 'disabled';
    document.getElementById('heat_slider').disabled = true;
    document.getElementById('fan_slider').className = 'disabled';
    document.getElementById('fan_slider').disabled = true;
    document.getElementById('stir_slider').className = 'disabled';
    document.getElementById('stir_slider').disabled = true;
  } else {
    document.getElementById('closed_loop').innerHTML = 'Turn closed-loop temperature control ON';
    document.getElementById('heat_slider').className = 'enabled';
    document.getElementById('heat_slider').disabled = false;
    document.getElementById('fan_slider').className = 'enabled';
    document.getElementById('fan_slider').disabled = false;
    document.getElementById('stir_slider').className = 'enabled';
    document.getElementById('stir_slider').disabled = false;
  }
}

function pause_resume() {
  system_active = !system_active;
  var command = 'p';
  console.log(command);
  connection.send(command);
  pause_button();
}

function pause_button() {
  if (system_active) {
    document.getElementById('pause').innerHTML = 'Pause';
  } else {
    document.getElementById('pause').innerHTML = 'Resume';
  }
}

function draw(data, id, ymax) {
  // References
  // https://bl.ocks.org/mbostock/3885304
  // http://bl.ocks.org/Jverma/raw/887877fc5c2c2d99be10/

  // remove any pre-existing graphics
  d3.select(id).select("svg").remove();

  // set the dimensions of the canvas
  var margin = {top: 20, right: 20, bottom: 50, left: 40},
      width = data.length * 50 + margin.left + margin.right,
      height = 300 - margin.top - margin.bottom;

  // set the ranges
  var x = d3.scale.ordinal().rangeRoundBands([0, width], .05);
  var y = d3.scale.linear().range([height, 0]);

  // define the axis
  var xAxis = d3.svg.axis()
      .scale(x)
      .orient("bottom")

  var yAxis = d3.svg.axis()
      .scale(y)
      .orient("left")
      .ticks(10);

  // add the SVG element
  var svg = d3.select(id).append("svg")
      .attr("width", width + margin.left + margin.right)
      .attr("height", height + margin.top + margin.bottom)
    .append("g")
      .attr("transform", 
            "translate(" + margin.left + "," + margin.top + ")");
  
  // scale the range of the data
  x.domain(data.map(function(d) { return d.label; }));
  y.domain([0, ymax]);

  // add axis
  svg.append("g")
      .attr("class", "x axis")
      .attr("transform", "translate(0," + height + ")")
      .call(xAxis)
    .selectAll("text")
      .style("text-anchor", "end")
      .style("font", "18px")
      .attr("dx", "1em")
      .attr("dy", "1em")
      // .attr("transform", "rotate(-90)" );

  svg.append("g")
      .attr("class", "y axis")
      .style("font", "18px")
      .call(yAxis)

  // Add bar chart
  svg.selectAll("bar")
      .data(data)
    .enter().append("rect")
      .attr("class", "bar")
      .attr("x", function(d) { return x(d.label); })
      .attr("width", x.rangeBand())
      .attr("y", function(d) { return y(d.y); })
      .attr("height", function(d) { return height - y(d.y); });
}