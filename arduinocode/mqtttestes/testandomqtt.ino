#include <SPI.h>
#include <NewPing.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define fim_curso_aberto 52
#define fim_curso_fechado 22
#define TRIGGER_PIN1  30
#define ECHO_PIN1     28
#define MAX_DISTANCE 200

#define AIO_SERVER      "mqtt.sj.ifsc.edu.br"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Alisson"
#define AIO_KEY         ""

//Definicoes pinos Arduino ligados a entrada da Ponte H (para testes o motor está em A, mesmo as variáveis estando para o motor B)
int IN3 = 36;
int IN4 = 38;
int vel = 34;


byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient client;
NewPing sonar1(TRIGGER_PIN1, ECHO_PIN1, MAX_DISTANCE);

const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;

const char MQTT_CLIENTID[] PROGMEM  = __TIME__ AIO_USERNAME;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME); //cria client mqtt


Adafruit_MQTT_Publish pubstatus = Adafruit_MQTT_Publish(&mqtt, "/porta/status"); //instacia do objeto para publicar em topico /porta/status

Adafruit_MQTT_Subscribe subcomandos = Adafruit_MQTT_Subscribe(&mqtt, "/porta/comandos"); //instancia do objeto para se inscrever em topico /porta/comandos


char *doorcomand[1]; //1 - abre/fecha, 2- abre (implementar), 3 - fecha (implementar)

uint32_t doorstatus = 0; //1-aberto, 0-fechado, 2-abrindo, 3-fechando;

void setup() {

Serial.begin(9600);
pinMode(IN3, OUTPUT);
pinMode(IN4, OUTPUT);
pinMode(vel, OUTPUT);
pinMode(fim_curso_aberto, INPUT);
pinMode(fim_curso_fechado, INPUT);
pinMode(5, OUTPUT);
pinMode(6, OUTPUT);
pinMode(7, OUTPUT);


Serial.println("Setup concluido");

Serial.print(F("\nInicializando o cliente...."));
Ethernet.begin(mac);
delay(1000); 


mqtt.subscribe(&subcomandos); //inscreve no topico /porta/comandos

}

void loop() {

  digitalWrite(7, HIGH); //acende vermelho
  
  MQTT_connect(); //rotina conecta mqtt


//---- config motor (estudar) -------//
  int aberto = digitalRead(fim_curso_aberto);
  int fechado = digitalRead(fim_curso_fechado);
  float distancia = sonar1.ping_cm();

  char porta_fechada_str[10];
  char porta_aberta_str[10];
   
  itoa(fechado, porta_fechada_str, 10);
  itoa(aberto, porta_aberta_str, 10);
//----------------------------------------//


  publicar(doorstatus); //rotina publicar para o status atual da porta
  
    
  subscriber(); //rotina para verificar se chegou algum dado do tópico inscrito
  
  if ((distancia > 0 && distancia <= 30) || *doorcomand[0] == '1'){ //testa o valor da variavel doorcomand para saber o que fazer
    digitalWrite(7, LOW); //apaga led vermelho fechado
    digitalWrite(6, HIGH); // acende led amarelo abrindo
    doorstatus = 2; //abrindo
    publicar(doorstatus);
    delay(5000);
    Serial.println("ABRIR PORTA");
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(vel, 220);
    delay(5000);
    while (aberto == 1) {
      aberto = digitalRead(fim_curso_aberto);
    }
    digitalWrite(6, LOW); //apaga led amarelo
    digitalWrite(5, HIGH); //Acende led verde aberto
    Serial.println("FIM DE CURSO ABERTO");
    doorstatus = 1; //aberta
    publicar(doorstatus);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, HIGH);
    

    delay(5000);

    Serial.println("FECHAR PORTA");
    doorstatus = 3; //fechando
    publicar(doorstatus);
    digitalWrite(6, HIGH); //acende led amarelo
    digitalWrite(5, LOW); //apaga led verde aberto
    delay(5000);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(vel, 220);
    while (fechado == 1) {
      fechado = digitalRead(fim_curso_fechado);
    }
    Serial.println("FIM DE CURSO FECHADO");
    digitalWrite(6, LOW); //apaga amarelo
    digitalWrite(7, HIGH); //acende vermelho
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, HIGH);
    doorstatus = 0; //fechada
    publicar(doorstatus);
    *doorcomand[0] = '0';
    delay(5000);
  }

}
void publicar(uint32_t x) {
Serial.print(F("\nEnviando status "));
Serial.print(x);
Serial.print("...");
if (! pubstatus.publish(x++)) {
  Serial.println(F("Falhou"));
} else {
  Serial.println(F("OK!"));
}
}


void MQTT_connect() {
int8_t ret;

if (mqtt.connected()) {
  return;
}

Serial.print("Conectando ao MQTT... ");

while ((ret = mqtt.connect()) != 0) { 
     Serial.println(mqtt.connectErrorString(ret));
     Serial.println("Tentando reconexão em 3 segundos...");
     mqtt.disconnect();
     delay(3000);  // wait 5 seconds
}
Serial.println("MQTT Connected!");
}

void subscriber() {
Adafruit_MQTT_Subscribe *subscription;
while ((subscription = mqtt.readSubscription(1000))) {
  if (subscription == &subcomandos ) {
    Serial.print(F("Chegou: "));
    doorcomand[0] = (char *)subcomandos.lastread; //valor que chega vai para variavel doorcomand
    Serial.println((char *)subcomandos.lastread); //exibe na tela comando;
  } 
  }
  }


