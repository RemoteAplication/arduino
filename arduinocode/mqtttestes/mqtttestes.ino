/***************************************************
  Adafruit MQTT Library Ethernet Example

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Alec Moore
  Derived from the code written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <SPI.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include <Ethernet.h>
#include <EthernetClient.h>
#include <Dns.h>
#include <Dhcp.h>

/************************* Ethernet Client Setup *****************************/
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};


/************************* Config *********************************/

#define AIO_SERVER      "mqtt.sj.ifsc.edu.br"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "arduino"

#define fim_curso_aberto 52
#define fim_curso_fechado 22
#define TRIGGER_PIN1  30
#define ECHO_PIN1     28
#define MAX_DISTANCE 200

int IN3 = 36;
int IN4 = 38;
int vel = 34;



EthernetClient client; //inicia cliente ethernet

const char MQTT_SERVER[] PROGMEM = AIO_SERVER;

const char MQTT_CLIENTID[] PROGMEM  = __TIME__ AIO_USERNAME; //cliente id = horario+username
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;


Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME);


//#define halt(s) { Serial.println(F( s )); while(1);  }


/****************************** Publica ***************************************/

const char publish[] PROGMEM = "arduino/status";
Adafruit_MQTT_Publish pubstatus = Adafruit_MQTT_Publish(&mqtt, publish);

/****************************** Inscreve ***************************************/
const char comandos[] PROGMEM = "arduino/comandos";
Adafruit_MQTT_Subscribe subcomandos = Adafruit_MQTT_Subscribe(&mqtt, comandos);

/*************************** Sketch Code ************************************/

NewPing sonar1(TRIGGER_PIN1, ECHO_PIN1, MAX_DISTANCE);

void setup() {
  Serial.begin(115200);

  Serial.print(F("\nInicializando Cliente MQTT"));
  Ethernet.begin(mac);
  delay(1000);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(vel, OUTPUT);
  pinMode(fim_curso_aberto, INPUT);
  pinMode(fim_curso_fechado, INPUT);
  

  mqtt.subscribe(&subcomandos);
}

uint32_t x=0;

void loop() {

  MQTT_connect(); //funcao conecta no mqtt

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000))) { //espera por dados do topico inscrito
    if (subscription == &subcomandos) {
      Serial.print(F("Got: "));
      Serial.println((char *)subcomandos.lastread);
    }
  }

  // Now we can publish stuff!
  Serial.print(F("\nEnviando valor"));
  Serial.print(x);
  Serial.print("...");
  if (! pubstatus.publish(x++)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  if(! mqtt.ping()) { //ping para servidor mqtt para verificar conexao
    mqtt.disconnect();
  }

}

//funcao conecta no mqtt server
void MQTT_connect() {
  int8_t ret;

  if (mqtt.connected()) { // termina se já conectado
    return;
  }

  Serial.print("Conectando ao servidor MQTT...");

  while ((ret = mqtt.connect()) != 0) { // Retorna 0 se conectado
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Tentando conexão novamente em 5 segundos...");
       mqtt.disconnect();
       delay(5000); 
  }
  Serial.println("MQTT Conectado!");
}
