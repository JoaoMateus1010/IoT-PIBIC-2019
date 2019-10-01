#include <ESP8266WiFi.h>
#include "lib/adc/EmonLib-master/EmonLib.h"
#include "lib/mqtt/pubsubclient-master/src/PubSubClient.h"
#include "defines.h"

#define TOPICO_SUBSCRIBE "MQTTPibicIotEscuta"     //tópico MQTT de escuta
#define TOPICO_PUBLISH   "MQTTPibicIotRecebe"    //tópico MQTT de envio de informações para Broker
                                                   //IMPORTANTE: recomendamos fortemente alterar os nomes
                                                   //            desses tópicos. Caso contrário, há grandes
                                                   //            chances de você controlar e monitorar o NodeMCU
                                                   //            de outra pessoa.
#define ID_MQTT  "HomeAut"     //id mqtt (para identificação de sessão)
                               //IMPORTANTE: este deve ser único no broker (ou seja, 
                               //            se um client MQTT tentar entrar com o mesmo 
                               //            id de outro já conectado ao broker, o broker 
                               //            irá fechar a conexão de um deles).

//GLOBAL VARIAVEIS
char EstadoSaida = '0';
//Pino
int pinSCT = A0;

//Instância do sensor
EnergyMonitor SCT013;

//Tensão e potência
int tensao = VOLTAGE_220;
int potencia = 0;

//WIFI
char* ssid = SSID_CLIENT;
char* password = PASSWORD_ClIENT;

//MQTT
const char* BROKER_MQTT = "mqtt.fluux.io"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

//Funções

//Serial init
void initSerial(){
  Serial.begin(9600);
}

//Wifi Client init
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

//init MQTT
void initMQTT(){
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}
void EnviaEstadoOutputMQTT(float corrente, float potencia){
    char* val = "Teste";
    MQTT.publish(TOPICO_PUBLISH, val);
    Serial.println("- Estado da saida D0 enviado ao broker!");
    delay(1000);
}
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
        
}
void reconnectMQTT() 
{
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
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
   
    //toma ação dependendo da string recebida:
    //verifica se deve colocar nivel alto de tensão na saída D0:
    //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
    //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
    if (msg.equals("L"))
    {
        digitalWrite(D0, LOW);
        EstadoSaida = '1';
    }
 
    //verifica se deve colocar nivel alto de tensão na saída D0:
    if (msg.equals("D"))
    {
        digitalWrite(D0, HIGH);
        EstadoSaida = '0';
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

//Calcula potência
double calcPower(int VOLTAGE,double Irms){
  return VOLTAGE*Irms;
  }  


// Configurações iniciais 
void setup(){  
initSerial();
initWIFIClient();
initMQTT();
initSCT013();
}


//Repetidor
void loop(){    
    double currentIrms = readIrms();    
    int currentPower = calcPower(tensao,currentIrms);
    Serial.print("Corrente = ");
    Serial.print(currentIrms);
    Serial.println(" A");
    
    Serial.print("Potencia = ");
    Serial.print(currentPower);
    Serial.println(" W");
    VerificaConexoesWiFIEMQTT();
 
    //envia o status de todos os outputs para o Broker no protocolo esperado
    EnviaEstadoOutputMQTT(currentIrms,currentPower);
 
    //keep-alive da comunicação com broker MQTT
    MQTT.loop();
    delay(1000);
}
