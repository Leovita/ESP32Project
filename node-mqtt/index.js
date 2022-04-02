const mqtt = require("mqtt");
const { MongoClient, ServerApiVersion } = require("mongodb");
const nodemailer = require("nodemailer");
const http = require("http");
require("dotenv").config();

const host = "mqtt.ssh.edu.it";
const mqttPort = "1883";
const topic = "museum/params";
const serverPort = 3000;
const clientId = `mqtt_${Math.random().toString(16).slice(3)}`;
const connectUrl = `mqtt://${host}:${mqttPort}`;
let collection = "IotProjectESP32";

const client = mqtt.connect(connectUrl, {
  clientId,
  clean: true,
  connectTimeout: 4000,
  username: "",
  password: "",
  reconnectPeriod: 1000,
});

let transporter = nodemailer.createTransport({
  service: "gmail",
  auth: {
    user: "leonardo.vitale@fermi.mo.it",
    pass: process.env.MAILPASS,
  },
});

let mailOptions = {
  from: "leonardo.vitale@fermi.mo.it",
  to: "leonardo.vitale@fermi.mo.it",
  subject: "Alert e-mail from louvre museum",
  text: "Hi, this e-mail got automatically sent from the fire prevention system, please check musem state as soon as possible",
};

client.on("connect", () => {
  console.log("Connected");
  client.subscribe([topic], () => {
    console.log(`Subscribe to topic '${topic}'`);
  });
});

function jsonCheck(jsonObj) {
  for (var key in jsonObj) {
    if (jsonObj[key] == null || jsonObj[key] == "undefined") {
      collection = "badJsonFormats";
      break;
    } else {
      collection = "IotProjectESP32";
    }
  }
}

client.on("message", (topic, payload) => {
  console.log("Received Message:", topic, payload.toString());
  let parsedJSON = JSON.parse(payload);
  jsonCheck(parsedJSON);
  MongoClient.connect(process.env.URISTRING, function (err, db) {
    if (err) throw err;

    if (parsedJSON.fireAlert) {
      transporter.sendMail(mailOptions, function (error, info) {
        if (error) {
          console.log(error);
        } else {
          console.log("Email sent: " + info.response);
        }
      });
    }
    let dbo = db.db("IotProjectESP32");
    dbo.collection(collection).insertOne(parsedJSON, function (err, res) {
      if (err) throw err;
      console.log("\n Json obj added to mongoDB!");
      db.close();
    });
  });
});
