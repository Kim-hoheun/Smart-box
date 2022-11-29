package com.example.smartbox;

import android.os.Bundle;
import android.content.Intent;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.DisconnectedBufferOptions;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import androidx.appcompat.app.AppCompatActivity;



public class MainActivity extends AppCompatActivity {

    private String ServerIP = "tcp://broker.mqtt-dashboard.com:1883";
    private String TOPIC_door = "Door";  //
    private String TOPIC_temper = "Temper";
    private String TOPIC_fan = "Fan_1";
    private String TOPIC_temper_respon = "Temper_respon";
    private String TOPIC_humi_respon = "Humi_respon";
    private String TOPIC_weight_respon = "Weight_respon";
    private String TOPIC_door_state_s = "Door_state_s";
    private MqttClient mqttClient = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Button btn_open = findViewById(R.id.btn_open);
        Button btn_close = findViewById(R.id.btn_close);
        TextView txt_temper = findViewById(R.id.txt_temper);
        TextView txt_humi = findViewById(R.id.txt_humi);
        ImageView image_view = findViewById(R.id.image_view);
        ImageView image_view2 = findViewById(R.id.image_view2);

        try {
            mqttClient = new MqttClient(ServerIP, MqttClient.generateClientId(), null);
            mqttClient.connect();

            btn_open.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    try {
                        mqttClient.publish(TOPIC_door, new MqttMessage("open12".getBytes()));  //open 버튼 눌렀을때 "Door" 토픽에 "open12" 퍼블리시하여 문 열림 제어
                        mqttClient.publish(TOPIC_fan, new MqttMessage("fan_off".getBytes()));  //open 버튼 눌렀을때 "Fan_1" 토픽에 "fan_off" 퍼블리시하여 팬 중단 제어
                    } catch (MqttException e) {
                        e.printStackTrace();
                    }
                }
            });

            btn_close.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    try {
                        mqttClient.publish(TOPIC_door, new MqttMessage("close".getBytes()));  //close 버튼 눌렀을때 Door 토픽에 "close" 퍼블리시하여 문 닫힘 제어
                        mqttClient.publish(TOPIC_fan, new MqttMessage("fan_on".getBytes()));  //close 버튼 눌렀을때 Fan_1 토픽에 "fan_on" 퍼블리시하여 팬 동작 제어
                    } catch (MqttException e) {
                        e.printStackTrace();
                    }
                }
            });

            class NewRunnable implements Runnable {
                @Override
                public void run() {
                    while (true) {
                        try{
                            mqttClient.publish(TOPIC_temper, new MqttMessage("temper_data".getBytes()));  // 1분마다 Temper 토픽에 온도,습도값 퍼블리시
                        } catch (MqttException e){
                            e.printStackTrace();
                        }

                        try {
                            Thread.sleep(60000) ;
                        } catch (Exception e) {
                            e.printStackTrace() ;
                        }
                    }
                }
            }
            NewRunnable nr = new NewRunnable() ;
            Thread t = new Thread(nr) ;
            t.start() ;

            mqttClient.subscribe(TOPIC_temper_respon); //아두이노가 보내는 온도값 받기 위해 "Temper_respon" 토픽 구독
            mqttClient.subscribe(TOPIC_humi_respon); //아두이노가 보내는 습도값 받기 위해 "Humi_respon" 토픽 구독
            mqttClient.subscribe(TOPIC_weight_respon); //아두이노가 보내는 무게값 받기 위해 "Weight_respon" 토픽 구독
            mqttClient.subscribe(TOPIC_door_state_s); //아두이노가 보내는 문 열림,닫힘 상태 확인을 위해 "Temper_respon" 토픽 구독

            mqttClient.setCallback(new MqttCallback() {
                @Override
                public void connectionLost(Throwable throwable) {
                    Log.d("MQTTService", "Connection Lost");
                }
                @Override
                public void messageArrived(String s, MqttMessage mqttMessage) throws Exception {
                    String msg = new String(mqttMessage.getPayload());
                    String recieve = mqttMessage.toString(); // 구독한 토픽에서 받아온 mqttMessage를 String형태로 변환
                    String str = recieve.substring(2); // 받아온 값 중에 알수없는 값이 섞여있어 parsing 사용
                    Log.d("MQTTService", "Message Arrived : " + str);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (s.equals(TOPIC_temper_respon)){ // 보내온 값의 토픽이 "Temper_respon"인 경우
                                txt_temper.setText("온도 : " + str + "℃"); // textview에 온도 : 00℃ 형태로 출력
                            }
                            else if(s.equals(TOPIC_humi_respon)){ // 보내온 값의 토픽이 "Humi_respon"인 경우
                                txt_humi.setText("습도 : " + str + "%"); // textview에 습도 : 00% 형태로 출력
                            }
                            else if(s.equals(TOPIC_weight_respon)){ // 보내온 값의 토픽이 "Weight_respon"인 경우
                                if(str.equals("none")){ // 토픽에서 보내온 값이 "none"이면 물건이 없는 상황이다.
                                    image_view.setImageResource(R.drawable.delivery_2); // 따라서 imageview에 배송중 이미지 출력
                                }
                                else{ // 값이 "none"이 아닌 경우 물건이 있는 상황이다.
                                    image_view.setImageResource(R.drawable.box_2); // 따라서 imageview에 배송완료 이미지 출력
                                }
                            }
                            else if(s.equals(TOPIC_door_state_s)){ // 보내온 값의 토픽이 "Door_state_s"인 경우
                                if(str.equals("opened")){ // 토픽에서 보내온 값이 "opened"인 경우 문이 열려있는 상황이다.
                                    image_view2.setImageResource(R.drawable.unlocked); // 따라서 imageview에 문 열림 이미지 출력
                                }
                                else{ // "opened" 이외의 값이 들어오면 문이 닫혀있는 상황이다.
                                    image_view2.setImageResource(R.drawable.locked); // 따라서 imageview에 문 닫힘 이미지 출력
                                }
                            }
                        }
                    });
                }
                @Override
                public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {
                    Log.d("MQTTService", "Delivery Complete"); // 버튼을 눌렀을 때 토픽에 퍼블리시가 되면 Logcat에 MQTTService 태그로 메세지 출력
                }
            });
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }
}