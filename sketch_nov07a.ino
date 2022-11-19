//Serial1이라고 쓰면 Serial하고 햇갈리니까 이름을 바꾸겠다
#define ESP01 Serial1
#define DHTPIN 2 // 2번핀 온습도핀으로 설정, 2번으로 데이터가 들어온다.
#define DHTTYPE DHT22 // 온습도 함수 매개변수를 위해 DHT22라는 타입을 DHTTYPE으로 치환

#include<Servo.h> //Servo 라이브러리
#include <DHT.h> // 온습도 라이브러리

Servo servo;                  //Servo class 객체
DHT dht(DHTPIN, DHTTYPE);     // 2번핀과 데이터 타입을 매개변수로 dht 선언 -> 온습도

int FSRsensor = A0;           // 압력센서값을 아나로그 A0핀 설정
int value;        //서브모터 각도를 조절할 변수
int fan = 4;         // 펜 4번으로 출력신호를 내보냄
int tem = 0; // 온도값
int h = 0; //습도값

bool somethingOn = false; // 압력세서 if문을 제어하기 위한 변수
bool btn = false; // 버튼을 제어하기 위한 변수

bool is_MQTT_CONNECTED = false;
unsigned long t = 0;
unsigned long t1 = 0;
unsigned int mycount = 0;


void setup() {
  pinMode(13,INPUT_PULLUP); //  버튼을 13번핀으로 연결 
  servo.attach(6); // servo 서보모터를 6번 핀으로 제어한다.
  servo.write(0);  // 서브모터 0도로 초기값 초기화
  pinMode(fan, OUTPUT); // 4번핀을 fan의 출력핀으로
  
  Serial.begin(115200); //PC-아두이노간 통신
  ESP01.begin(115200); //아두이노-ESP01

  while (!ESP01) {
    ; // wait for serial port to connect.
  }

  //인터넷 공유기 접쏚!
  NOCKANDA_WIFI("1-419","");
  
}

void loop() {
  
  
  
  if(!is_MQTT_CONNECTED){
    //setup에서 최초로 MQTT브로커와 연결한 이후로 서버와 단절되었음
    reconnect();
  }

  int sensorval = analogRead(A0);                    // 압력센서 값을 A0로 받아온다
  if(sensorval >150 && !somethingOn){                                 // 압력센서 값이 50보다 크다 == "뭔가 올라가 있다" 
    NOCKANDA_PUBLISH("Weight_respon", String(sensorval).c_str()); // 그렇다면 압력센서 값을 전송
    somethingOn = true;
    Serial.println("1");
    Serial.println(sensorval);
  }else if(sensorval < 50 && somethingOn){
    NOCKANDA_PUBLISH("Weight_respon", String("none").c_str()); // 그렇다면 압력센서 값을 전송
    somethingOn = false;
    Serial.println("2");
    Serial.println(sensorval);
  }

  if(digitalRead(13)){
    btn = true;
    if(btn){
      digitalWrite(fan, HIGH);
      value = 90;
      servo.write(value);
      NOCKANDA_PUBLISH("Door_state_s", String("closed").c_str());
      Serial.print("d");
    }
  }
  
  //ESP01에서 수신된 데이터를 시리얼모니터로 전달~
  if(ESP01.available()){
    String data = ESP01.readStringUntil('\n');
    if(data.indexOf("CLOSED") != -1){
      //MQTT브로커와 연결이 끊어졌다!
      is_MQTT_CONNECTED = false;
    }else if(data.indexOf("+IPD") != -1){
      //Serial.print("구독등록한 메시지입니다=");
      //16  9토픽의 길이 5
      NOCKANDA_MSG_PROCESS(data.c_str(), sensorval);
      
    }
  }
}

void reconnect(){
  NOCKANDA_CONNECT(); //MQTT 브로커와 연결이 완료!
  //구독할 토픽을 지정한다!
  NOCKANDA_SUBSCRIBE("Door");
  NOCKANDA_SUBSCRIBE("Temper");
  NOCKANDA_SUBSCRIBE("Weight");
  NOCKANDA_SUBSCRIBE("Humi");
  NOCKANDA_SUBSCRIBE("Fan_1");
  NOCKANDA_SUBSCRIBE("Door_state");
}

void NOCKANDA_WIFI(char * id, char * pw){
  //인터넷공유기와 니 알아서 연결하세요!
  ESP01.write("AT+CWJAP=\"");
  ESP01.write(id);
  ESP01.write("\",\"");
  ESP01.write(pw);
  ESP01.write("\"");
  ESP01.println(); //CR+LF
  
  //WIFI CONNECTED
  //WIFI GOT IP
  //OK
  int mystep = 0;
  while(true){
    if(ESP01.available()){
      String data = ESP01.readStringUntil('\n');//readline 
      if(data.indexOf("WIFI CONNECTED") != -1 && mystep == 0){
        Serial.println("인터넷 공유기 접속완료!");
        mystep++;
      }else if(data.indexOf("WIFI GOT IP") != -1 && mystep == 1){
        Serial.println("IP주소 획득!");
        mystep++;
      }else if(data.indexOf("OK") != -1 && mystep == 2){
        Serial.println("WIFI접속완료!");
        break;
      }
    }
  }
}

void NOCKANDA_CONNECT(){
  //MQTT브로커와 TCP연결
  ESP01.println("AT+CIPSTART=\"TCP\",\"broker.mqtt-dashboard.com\",1883");
  //CONNECT
  //OK
  int mystep = 0;
  while(true){
    if(ESP01.available()){
      String data = ESP01.readStringUntil('\n');//readline 
      if(data.indexOf("CONNECT") != -1 && mystep == 0){
        Serial.println("CONNECT 응답 수신");
        mystep++;
      }else if(data.indexOf("OK") != -1 && mystep == 1){
        Serial.println("OK 응답 수신");
        break;
      }
    }
  }
  Serial.println("MQTT브로커와 TCP연결완료!");
  ESP01.println("AT+CIPSEND=22");
  //OK
  //>
  mystep = 0;
  while(true){
    if(ESP01.available()){
      String data = ESP01.readStringUntil('\n');//readline 
      if(data.indexOf("OK") != -1 && mystep == 0){
        Serial.println("OK 응답 수신");
        mystep++;
      }else if(data.indexOf(">") != -1 && mystep == 1){
        Serial.println("> 응답 수신");
        break;
      }
    }
  }
  Serial.println("데이터 업로드 준비완료!");
  byte num1 = random(0,10); 
  byte num2 = random(0,10);
  byte num3 = random(0,10);
  byte num4 = random(0,10);
  byte MQTT_CONNECT[] = {0x10,0x14,0x00,0x04,'M','Q','T','T',0x04,0x02,0x00,0x3C,0x00,0x08,'N','O','C','K','0'+num1,'0'+num2,'0'+num3,'0'+num4};
  //MQTT브로커와 커넥션(MQTT CONNECT DATA FRAME)(1)
  ESP01.write(MQTT_CONNECT,22);
  //Recv 22 bytes
  //SEND OK
  //+IPD,4
  mystep = 0;
  while(true){
    if(ESP01.available()){
      String data = ESP01.readStringUntil('\n');//readline 
      if(data.indexOf("Recv") != -1 && mystep == 0){
        Serial.println("RECV 응답 수신");
        mystep++;
      }else if(data.indexOf("SEND") != -1 && mystep == 1){
        Serial.println("SEND 응답 수신");
        mystep++;
      }else if(data.indexOf("+IPD") != -1 && mystep == 2){
        Serial.println("+IPD 응답 수신");
        break;
      }
    }
  }
  Serial.println("MQTT브로커와 연결이 완료되었습니다!");
  is_MQTT_CONNECTED = true;
}

void NOCKANDA_PUBLISH(char * topic, char * payload){
  int topic_length = NOCKANDA_LENGTH(topic);
  int payload_length = NOCKANDA_LENGTH(payload);
  //Serial.println("PUBLISH 시작!!");
  ESP01.println("AT+CIPSEND="+String(6+topic_length+payload_length));
  //OK
  //> 
  int mystep = 0;
  while(true){
    int sensorval = analogRead(A0);
    if(ESP01.available()){
      String data = ESP01.readStringUntil('\n');//readline 
      if(data.indexOf("+IPD") != -1){
        //발행하는 와중에 메시지가 수신된 독특한경우
        NOCKANDA_MSG_PROCESS(data.c_str(), sensorval);
      }
      if(data.indexOf("OK") != -1 && mystep == 0){
        //Serial.println("OK 응답 수신");
        mystep++;
      }else if(data.indexOf(">") != -1 && mystep == 1){
        //Serial.println("> 응답 수신");
        break;
      }
    }
  }
  //Serial.println("TCP업로드 준비 완료!!");
  //byte MQTT_PUBLISH[] = {0x30,0x19,0x00,0x08,'N','O','C','K','A','N','D','A',0x00,0x0D,'H','E','L','L','O',' ','W','O','R','L','D','!','!'};
  byte header1[] = {0x30,4+topic_length+payload_length,0x00,topic_length};
  byte header2[] = {0x00,payload_length};
  //header1
  //topic
  //header2
  //payload
  ESP01.write(header1,4);
  ESP01.write(topic,topic_length);
  ESP01.write(header2,2);
  ESP01.write(payload,payload_length);
  //Recv 27 bytes
  //SEND OK
  mystep = 0;
  while(true){
    int sensorval = analogRead(A0);
    if(ESP01.available()){
      String data = ESP01.readStringUntil('\n');//readline 
      if(data.indexOf("+IPD") != -1){
        //발행하는 와중에 메시지가 수신된 독특한경우
        NOCKANDA_MSG_PROCESS(data.c_str(), sensorval);
      }
      if(data.indexOf("Recv") != -1 && mystep == 0){
        //Serial.println("Recv 응답 수신");
        mystep++;
      }else if(data.indexOf("SEND") != -1 && mystep == 1){
        //Serial.println("SEND OK 응답 수신");
        break;
      }
    }
  }
  Serial.println("MQTT PUBLISH완료!!");
}

void NOCKANDA_SUBSCRIBE(char * topic){
  int topic_length = NOCKANDA_LENGTH(topic);
  Serial.println("MQTT SUBSCRIBE시작!!");
  ESP01.println("AT+CIPSEND="+String(7+topic_length));
  //OK
  //>
  int mystep = 0;
  while(true){
    if(ESP01.available()){
      String data = ESP01.readStringUntil('\n');//readline 
      if(data.indexOf("OK") != -1 && mystep == 0){
        Serial.println("OK 응답 수신");
        mystep++;
      }else if(data.indexOf(">") != -1 && mystep == 1){
        Serial.println("> 응답 수신");
        break;
      }
    }
  }
  Serial.println("데이터 업로드 준비완료!!");
  //byte MQTT_SUBSCRIBE[] = {0x82,0x0E,0x00,0x01,0x00,0x09,'N','O','C','K','A','N','D','A','1',0x00};
  byte header[] = {0x82,5+topic_length,0x00,0x01,0x00,topic_length};
  ESP01.write(header,6);
  ESP01.write(topic,topic_length);
  ESP01.write(0x00);
  //Recv 16 bytes
  //SEND OK
  //+IPD,5
  mystep = 0;
  while(true){
    if(ESP01.available()){
      String data = ESP01.readStringUntil('\n');//readline 
      if(data.indexOf("Recv") != -1 && mystep == 0){
        Serial.println("Recv 응답 수신");
        mystep++;
      }else if(data.indexOf("SEND") != -1 && mystep == 1){
        Serial.println("SEND OK 응답 수신");
        mystep++;
      }else if(data.indexOf("+IPD") != -1 && mystep == 2){
        Serial.println("+IPD 응답 수신");
        break;
      }
    }
  }
  Serial.println("MQTT SUBSCRIBE완료!!");
}

void NOCKANDA_MSG_PROCESS(char * data, int sensorval){
  int total_length = data[9];
  int topic_length = data[11];
  int payload_length = (total_length -2) - topic_length;
  
  char topic[topic_length+1];
  char payload[payload_length+1];
  for(int i = 0;i<topic_length;i++){
    topic[i] = data[12+i];
  }
  topic[topic_length] = '\0';
  for(int i = 0;i<payload_length;i++){
    payload[i] = data[12+topic_length+i];
  }
  payload[payload_length] = '\0';
  Serial.print("TOPIC=");
  Serial.print(topic);
  Serial.print(", PAYLOAD=");
  Serial.println(payload);
  Serial.println(sensorval);
  
  String payload_string = payload;
  if(payload_string == "open12"){
    value = 0;
    servo.write(value);
    NOCKANDA_PUBLISH("Door_state_s", String("opened").c_str());
    }
  else if(payload_string == "close"){
    value = 90;
    servo.write(value);
    NOCKANDA_PUBLISH("Door_state_s", String("closed").c_str());  
    }
  else if(payload_string == "fan_on"){
    digitalWrite(fan, HIGH);
    }
  else if(payload_string == "fan_off"){
    digitalWrite(fan,LOW);
  }
  else if(payload_string == "temper_data"){
    
    tem = dht.readTemperature();                       // 온도    입력받음 -> t
    h = dht.readHumidity();                          // 습도    입력받음 -> h 

    NOCKANDA_PUBLISH("Temper_respon", String(tem).c_str());
    NOCKANDA_PUBLISH("Humi_respon", String(h).c_str());
    Serial.println("3");
  }
  else if(payload_string == "humi_data"){
    
    
  }
}

int NOCKANDA_LENGTH(char * input){
  int mycount = 0;

  while(true){
    if(input[mycount] == '\0'){
      break;
    }
    mycount++;
  }
  
  return mycount;
}
