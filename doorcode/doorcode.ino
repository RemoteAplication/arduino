#include <QueueList.h>

#include <SPI.h>
#include <NewPing.h>
#include <Ethernet.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <MFRC522.h>


//sensores
#define fim_curso_aberto 2 //fio laranja, fio verde = gnd
#define fim_curso_fechado 3 //fio laranja
#define TRIGGER_PIN1  8
#define ECHO_PIN1     9
#define MAX_DISTANCE 200


//Constantes mqtt
#define AIO_SERVER      "mqtt.sj.ifsc.edu.br"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Arduino1"
#define AIO_KEY         ""

const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_CLIENTID[] PROGMEM  = AIO_USERNAME;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;


//Definicoes pinos Arduino ligados a entrada da Ponte H (para testes o motor está em A, mesmo as variáveis estando para o motor B)
int IN3 = 36;
int IN4 = 38;
int vel = 34;
//alisson
//Configs Ethernet
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient client;

//Inicia sensor sonar
NewPing sonar1(TRIGGER_PIN1, ECHO_PIN1, MAX_DISTANCE);

#define SS_PIN 53
#define RST_PIN 48
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

uint8_t qos = 2;
//Inicia valores para mqtt
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME); //cria client mqtt
const char PORTASTATUS[] PROGMEM = AIO_USERNAME "/porta/status"; //atribui tópico a ser publicado
Adafruit_MQTT_Publish pubstatus = Adafruit_MQTT_Publish(&mqtt, PORTASTATUS); //instacia do objeto para publicar em topico /porta/status
const char PORTACOM[] PROGMEM = AIO_USERNAME "/porta/comandos"; //atribui tópico a ser inscrito
Adafruit_MQTT_Subscribe subcomandos = Adafruit_MQTT_Subscribe(&mqtt, PORTACOM); //instancia do objeto para se inscrever em topico /porta/comandos


QueueList <char *> commandlist;

//Variaveis para controle da porta
char *doorcomand[1]; //1-abre/espera/fecha, 2-abre, 3-fecha;
uint32_t doorstatus = 3; //1-aberto, 2-fechando/abrindo, 3-fechado;

void setup() {




  Serial.begin(9600); //Inicia Serial
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
  //Configs pinagens
  pinMode(IN3, OUTPUT); //ponte H
  pinMode(IN4, OUTPUT); //ponte H
  pinMode(vel, OUTPUT); //ponte H
  pinMode(fim_curso_aberto, INPUT); //sensor aberto
  pinMode(fim_curso_fechado, INPUT); //sensor fechado
  pinMode(5, OUTPUT); //led verde
  pinMode(6, OUTPUT); //led amarelo
  pinMode(7, OUTPUT); //led vermelho

  Serial.println(F("\nInicializando o cliente...."));
  Ethernet.begin(mac); //inicia ethernet
  delay(1000);
  Serial.print(F("\nIniciado, com IP = "));
  Serial.println(Ethernet.localIP()); //exibe endereço ip obtido via dhcp

  mqtt.subscribe(&subcomandos); //inscreve no topico /porta/comandos
    
  



  
}

//iniciar o arduino sempre com a porta fechada!!

void loop() {

  if(doorstatus == 3) {
    digitalWrite(7, HIGH); //acende vermelho se estado fechado
  }

  MQTT_connect(); //rotina conecta mqtt
    float distancia = sonar1.ping_cm();
    Serial.println(distancia);
   //obtem distancia sentida pelo sonar

  publicar(doorstatus); //rotina publicar para o status atual da porta

  subscriber(); //rotina para verificar se chegou algum dado do tópico inscrito
  //Serial.println(distancia);
  
  if (! commandlist.isEmpty()){  // se lista de comandos não estiver fazia, desenfileira comando e atribui a porta;


    doorcomand[0] = commandlist.pop();
    Serial.print("Desenfileirou ");
    Serial.println(doorcomand[0]);

  
  } else {

    doorcomand[0] = 0;
    Serial.println("Fila de comandos vazia!");
    
  }

  if (*doorcomand[0] == '2' && doorstatus == 3){ //apenas abrirá se porta estiver fechada.
    portaAbrir(); //abre porta
  }

  if (*doorcomand[0] == '3' && doorstatus == 1){ //apenas fechará se porta estiver aberta
    portaFechar(); //fecha porta
  }

   if (((verificarfid()) || *doorcomand[0] == '1') && doorstatus == 3) { //apenas abrirá/fechará se porta já estiver fechada
   portaAbrir(); //abre porta
    delay(5000); //aguarda 5 segundos
    publicar(doorstatus);
   portaFechar(); //fecha porta
   }

}

void portaAbrir() {
  float distancia = sonar1.ping_cm();
  Serial.println("ABRIR PORTA");

  digitalWrite(IN3, HIGH); //config abre porta
  digitalWrite(IN4, LOW); //config abre porta
  analogWrite(vel, 220); //config abre porta

  digitalWrite(7, LOW); //apaga led vermelho fechado
  digitalWrite(6, HIGH); // acende led amarelo abrindo
  doorstatus = 2; //altera estado para abrindo

  while (distancia > 10) {
    if (distancia <29) {
      digitalWrite(6, LOW);
      delay(200);
      digitalWrite(6, HIGH);
    }
    if (distancia <20) {
     digitalWrite(6, LOW);
     delay(100);
     digitalWrite(6, HIGH);
    }
    if (distancia <15) {
     digitalWrite(6, LOW);
     delay(50);
     digitalWrite(6, HIGH);
    }
    
    Serial.println("Porta abrindo");
    distancia = sonar1.ping_cm();
    Serial.println(distancia);
    publicar(doorstatus); //publica estado da porta abrindo
  }

  digitalWrite(IN3, HIGH); //config parar porta
  digitalWrite(IN4, HIGH); //config parar porta

  digitalWrite(6, LOW); //apaga led amarelo
  digitalWrite(5, HIGH); //Acende led verde aberto

  Serial.println("------------- PORTA ABERTA ----------------");

  doorstatus = 1; //altera estado para aberta
  publicar(doorstatus);
  //*doorcomand[0] == '0'; //comando recebe 0 (nada)

}

void portaFechar() {

  Serial.println("FECHAR PORTA");

  digitalWrite(IN3, LOW);   //config fechar porta
  digitalWrite(IN4, HIGH); //config fechar porta
  analogWrite(vel, 220);  //config fechar porta

  digitalWrite(6, HIGH); //acende led amarelo
  digitalWrite(5, LOW); //apaga led verde aberto
  doorstatus = 2; //altera estado para fechando

  int fechado = digitalRead(fim_curso_fechado);
  while (fechado == 0) {
    Serial.println("Porta fechando");
    fechado = digitalRead(fim_curso_fechado); //leitura pino fechado
    publicar(doorstatus); //publica estado fechando
  }

  Serial.println("---------- PORTA FECHADA --------------");


  digitalWrite(IN3, HIGH); //config parar porta
  digitalWrite(IN4, HIGH); //config parar porta

  digitalWrite(6, LOW); //apaga amarelo
  digitalWrite(7, HIGH); //acende vermelho

  doorstatus = 3; //altera estado para fechada

  *doorcomand[0] = '0'; //altera comando para 0 (nada)

}

void publicar(uint32_t x) {

  Serial.print(F("\nEnviando status "));
  Serial.print(x);
  Serial.print("...");
  if (! pubstatus.publish(x)) { //escreve se conseguiu ou nao publicar
    Serial.println(F("Falhou"));
  } else {
    Serial.println(F("OK!"));
  }

}

void MQTT_connect() {
  int8_t ret;

  if (mqtt.connected()) { //se ja estiver conectado nao faz nada
    return;
  }

  Serial.print("Conectando ao MQTT... ");

  while ((ret = mqtt.connect()) != 0) { //enquanto não estiver conectado
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Tentando reconexão em 3 segundos...");
    mqtt.disconnect();
    delay(3000);
  }
  Serial.println("MQTT Conectado!");
}

void subscriber() {
  Adafruit_MQTT_Subscribe *subscription; //instancia objeto para receber valor do tópico subscrito
  while ((subscription = mqtt.readSubscription(1000))) {
    if (subscription == &subcomandos ) { //se recebeu algo
      Serial.print(F("Chegou: "));
      //doorcomand[0] = (char *)subcomandos.lastread ; //valor que chega vai para variavel doorcomand

      commandlist.push((char *)subcomandos.lastread); //comando que chega vai para a lista

      Serial.println((char *)subcomandos.lastread); //exibe na tela comando;
    }
  }
}

bool verificarfid() {

 if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return false ;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return false ;
  }
  String conteudo= "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  conteudo.toUpperCase();
  if (conteudo.substring(1) == "3D 39 62 62") //UID 1 - Chaveiro
  {
    return true;
  }
  
  if (conteudo.substring(1) == "10 9C 82 19") //UID 2 - Cartao
  {
    return true;
  }

  return false;
  
} 


