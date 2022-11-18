document.getElementById('light-on').addEventListener('click', function () {
    var ts_api_key = "ZFLCJT4LWWZJ5ODH";
    var url = "http://api.thingspeak.com/update?api_key=" + ts_api_key + "&field6=1"
    $.getJSON(url, function (data) {
        console.log(data);
    });
});

document.getElementById('light-off').addEventListener('click', function () {
    var ts_api_key = "ZFLCJT4LWWZJ5ODH";
    var url = "http://api.thingspeak.com/update?api_key=" + ts_api_key + "&field6=0"
    $.getJSON(url, function (data) {
        console.log(data);
    });
});

setInterval(function () {
    // Call a function repetatively with 15 Second interval
    getThingSpeakLightState();
    getThingSpeakTemprage();
    RainLeval();
    ldrMesurement();
    distance_cm();
    waterflowSpeed();
}, 15000);

function getThingSpeakLightState() {
    var ts_channel_id = 1935507;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            if (myObj.field6 == 1) {
                document.getElementById("LIGHTState").innerHTML = "ON";

            }
            else {
                document.getElementById("LIGHTState").innerHTML = "OFF";
            }
        }
    };
    xhttp.open("GET", "https://api.thingspeak.com/channels/" + ts_channel_id + "/fields/6/last.json", true);
    xhttp.send();
}
//get data from Thinks Speak server
//temprage data
function getThingSpeakTemprage() {
    var ts_channel_id = 1935507;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            document.getElementById("tempState").innerHTML = myObj.field1;
        }
    };
    xhttp.open("GET", "https://api.thingspeak.com/channels/" + ts_channel_id + "/fields/1/last.json", true);
    xhttp.send();
}
//RainLeval
function RainLeval() {
    var ts_channel_id = 1935507;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            document.getElementById("rainState").innerHTML = myObj.field2;
        }
    };
    xhttp.open("GET", "https://api.thingspeak.com/channels/" + ts_channel_id + "/fields/2/last.json", true);
    xhttp.send();
}
//LDR senser
function ldrMesurement() {
    var ts_channel_id = 1935507;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            document.getElementById("ldrState").innerHTML = myObj.field3;
        }
    };
    xhttp.open("GET", "https://api.thingspeak.com/channels/" + ts_channel_id + "/fields/3/last.json", true);
    xhttp.send();
}
//AltrasonicSencer
function distance_cm() {
    var ts_channel_id = 1935507;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            document.getElementById("distanceState").innerHTML = myObj.field4;
        }
    };
    xhttp.open("GET", "https://api.thingspeak.com/channels/" + ts_channel_id + "/fields/4/last.json", true);
    xhttp.send();
}
//waterflow
function waterflowSpeed() {
    var ts_channel_id = 1935507;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            document.getElementById("waterspeedState").innerHTML = myObj.field5;
        }
    };
    xhttp.open("GET", "https://api.thingspeak.com/channels/" + ts_channel_id + "/fields/5/last.json", true);
    xhttp.send();
}



