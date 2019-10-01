#include <ESP8266WiFi.h>
#include "EmonLib.h"
#include "import/WIFI/WIFI_DEFS.h"
#include "import/VOLTAGE_CURRENT_POWER/DEFS_VCP.h"
#include "import/MQTT/DEFINES_MQTT.h"
#include "src/PubSubClient.h"

/*--------------   GLOBAL VARIAVEIS  ----------------*/

// current values: curruent, power, error and calibration

double current  = 0.0;
int power    = 0.0;

double current_Error  = 0.0;
int power_Error    = 0.0;


//Pino
int pinSCT = A0;
int pinToggle = D1;

//Instância do sensor
EnergyMonitor SCT013;

//Tensão e potência
int tensao = VOLTAGE_220;

//WIFI
char* ssid = SSID_CLIENT;
char* password = PASSWORD_ClIENT;

//MQTT
const char* BROKER_MQTT   = DEFINE_BROKER_MQTT; 
int BROKER_PORT           = DEFINE_BROKER_PORT; 

WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

//Funções
//-----------------------> PIN init
void initPin(){
  pinMode(D1,OUTPUT);
}

//-----------------------> Serial init
void initSerial(){
  Serial.begin(9600);
}

//-----------------------> Wifi Client init
void initWIFIClient(){
  Serial.print("Tentando conexão...");
  WiFi.begin(ssid,password); 
  while (WiFi.status() != WL_CONNECTED){
    Serial.print("#");
    delay(1000);
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Conectado!");
}

//-----------------------> init MQTT
void initMQTT(){
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

// Publish and subscribe
void EnviaEstadoOutputMQTT(){
    String valorCorrente = String(current,3);
    String valorPotencia = String(power);    
    String valToSend = "{\"current\":"+valorCorrente+",\"power\":"+valorPotencia+"}";
    //Serial.print(valToSend);
    int sizel = 200;
    char sendValBuff[sizel];
    valToSend.toCharArray(sendValBuff,sizel);    
    MQTT.publish(TOPICO_PUBLISH, sendValBuff);    
    //Serial.println("Estado da saida A0 enviado ao broker!");
    delay(1000);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
   
    Serial.println("String recebida:");
    Serial.print(msg);
     
}
//-----------------------> Calibration 

void calibration(){  
  Serial.println("calibrating...");
  for(int i=0;i<NUMBER_TO_CALIBRATION;i++){
    digitalWrite(D1, !digitalRead(D1));  
    updateValues();
    current_Error += current;
    power_Error +=power;
    showValues();
    Serial.print("Step ");
    Serial.print(i);
    Serial.println(" done.");
    delay(1000);
  }
  current_Error /=  NUMBER_TO_CALIBRATION;
  power_Error   /=  NUMBER_TO_CALIBRATION;  
  Serial.println("Calibration done!");
  Serial.println(current_Error);
  Serial.println(power_Error);
  digitalWrite(D1, HIGH);
}
//Functions status
void VerificaConexoesWiFIEMQTT(void){
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita        
}
void reconnectMQTT() {
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
//Init Objeto relacionado ao sensor disponibilizado pela biblioteca
void initSCT013(){
  SCT013.current(pinSCT, 6.0606);
}

//Calcula Irms
double readIrms(){
  double Irms = 0.0;
  Irms = SCT013.calcIrms(1480);
  return Irms;
}
// Update values currente and power
void updateValues(){
  current = readIrms();
  power = current*tensao;  
  /*----- Tratamento inicial  -------*/
  //Analise corrente
  
}
//Show values
void showValues(){
    String valorCorrente = String(current,3);
    String valorPotencia = String(power);    
    String valToSend = "{\"current\":"+valorCorrente+",\"power\":"+valorPotencia+"}";
    Serial.println(valToSend);
}
/* ------------------  Configurações iniciais -----------------*/
void setup(){  
  initPin();
  initSerial();
  initWIFIClient();
  initMQTT();
  initSCT013();
  calibration();
}

/* ------------------  Laço -----------------*/
void loop(){    
  updateValues(); 
  showValues(); 
  VerificaConexoesWiFIEMQTT(); 
  //envia o status de todos os outputs para o Broker no protocolo esperado
  EnviaEstadoOutputMQTT();
  //keep-alive da comunicação com broker MQTT
  MQTT.loop(); 
  delay(1000);   
}
