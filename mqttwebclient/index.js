client = new Paho.MQTT.Client("mqtt.sj.ifsc.edu.br", Number("443"), "clientId");

client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

client.connect({ onSuccess: onConnect, useSSL: true });

function onConnect() {
  console.log("onConnect");
}

function onConnectionLost(responseObject) {
  if (responseObject.errorCode !== 0) {
    console.log("onConnectionLost:" + responseObject.errorMessage);
  }
}

function onMessageArrived(message) {
  console.log("onMessageArrived:" + message.payloadString);
  let node = document.createElement("LI");
  let textnode = document.createTextNode("Tópico: " + message.destinationName + ", mensagem " + message.payloadString);
  node.appendChild(textnode);
  document.getElementById("myList").appendChild(node);
}

function sendMessage() {
  topic = document.getElementById("topic").value.trim();
  client.subscribe(topic);
  message = new Paho.MQTT.Message(document.getElementById("message").value.trim());
  message.destinationName = topic;
  client.send(message);
}
