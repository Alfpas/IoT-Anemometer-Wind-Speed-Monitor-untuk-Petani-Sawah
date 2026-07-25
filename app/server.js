const express = require("express");
const http = require("http");
const { Server } = require("socket.io");
const mqtt = require("mqtt");

const app = express();
const server = http.createServer(app);
const io = new Server(server);

app.use(express.static("public"));
app.use("/img", express.static("img"));

const mqttClient = mqtt.connect({
    host: "t2a21271.ala.asia-southeast1.emqxsl.com",
    port: 8883,
    protocol: "mqtts",

    username: "anemometer_esp1",
    password: "esp12345678",

    rejectUnauthorized: false
});

mqttClient.on("connect", () => {

    console.log("MQTT Connected");

    mqttClient.subscribe("weather/anemometer");

});

mqttClient.on("message", (topic, message) => {

    try {

        const data = JSON.parse(message.toString());

        io.emit("sensor", data);

    } catch (e) {

        console.log(message.toString());

    }

});

server.listen(3000, () => {

    console.log("http://localhost:3000");

});