#include<Arduino.h>
#include <ArduinoBLE.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include<ArduinoJson.h>
#include <string.h>

const char* ssid ="Q3 Pro 5G";
const char* password =  "12345679";

#define MQTT_SERVER   "demo.thingsboard.io"
#define MQTT_PORT     1883
#define MQTT_USERNAME "80aP7J8ytfnaD058rMN9"      // Access token
#define MQTT_PASSWORD "123456"          // whatever u want
#define MQTT_NAME     "Client_Name"
#define NODE_1 "node_1"                    //khai bao ban tin "node1"
#define NODE_2 "node_2"                    //khai bao ban tin "node2"
#define NODE_3 "node_3"                     //khai bao ban tin "node3"   

WiFiClient client;
PubSubClient mqtt_client(client);

char* Temperature = "6";
String message = "";

String Node1 = "";
String Node2 = "";
String Node3 = "";

int maxTemp = 100;
int minTemp = 30;

static void monitorSensorTagButtons(BLEDevice peripheral);
static void setup_wifi(void);
static void callback(char* topic, byte *payload, unsigned int length); 
static void connectToBroker(void);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(callback);
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }
  connectToBroker();
  // start scanning for peripheral
  BLE.scan();
} 

void loop() {
  mqtt_client.loop();
  connectToBroker();
  // check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    // Serial.print("Found ");
    // Serial.print(peripheral.address());
    // Serial.print(" '");
    // Serial.print(peripheral.localName());
    // Serial.print("' ");
    // Serial.print(peripheral.advertisedServiceUuid());
    // Serial.println();

    // Check if the peripheral is a SensorTag, the local name will be:
    // "CC2650 SensorTag"
    if (peripheral.localName() == "Node-1") {
      // stop scanning
      BLE.stopScan();

      monitorSensorTagButtons(peripheral);
      Serial.print(Temperature[strlen(Temperature)-1]);
      String dataJS = "{\"Node1\":" + String(Temperature) + "}";
      char json[100];
      Serial.print(dataJS);
      Serial.print(String(Temperature));
      dataJS.toCharArray(json,dataJS.length()+1);
      mqtt_client.publish("v1/devices/me/telemetry",json);
      mqtt_client.publish("v1/devices/me/attributes",json);
 
      // peripheral disconnected, start scanning again
      BLE.scan();
      Temperature = "";
    }

    if (peripheral.localName() == "Node-2") {
      // stop scanning
      BLE.stopScan();

      monitorSensorTagButtons(peripheral);
      Serial.print(Temperature[strlen(Temperature)-1]);
      String dataJS = "{\"Node2\":" + String(Temperature) + "}";
      char json[100];
      Serial.print(dataJS);
      Serial.print(String(Temperature));
      dataJS.toCharArray(json,dataJS.length()+1);
      mqtt_client.publish("v1/devices/me/telemetry",json);
      mqtt_client.publish("v1/devices/me/attributes",json);
 
      // peripheral disconnected, start scanning again
      BLE.scan();
      Temperature = "";
    }

    if (peripheral.localName() == "Node-3") {
      // stop scanning
      BLE.stopScan();

      monitorSensorTagButtons(peripheral);
      Serial.print(Temperature[strlen(Temperature)-1]);
      String dataJS = "{\"Node3\":" + String(Temperature) + "}";
      char json[100];
      Serial.print(dataJS);
      Serial.print(String(Temperature));
      dataJS.toCharArray(json,dataJS.length()+1);
      mqtt_client.publish("v1/devices/me/telemetry",json);
      mqtt_client.publish("v1/devices/me/attributes",json);
 
      // peripheral disconnected, start scanning again
      BLE.scan();
      Temperature = "";
    }
  }
}

static void monitorSensorTagButtons(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");
  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering service 0xffe0 ...");
  if (peripheral.discoverService("ffe0")) {
    Serial.println("Service discovered");
  } else {
    Serial.println("Attribute discovery failed.");
    peripheral.disconnect();
    return;
  }

  // retrieve the simple key characteristic
  BLECharacteristic characteristic = peripheral.characteristic("ffe1");

  // subscribe to the simple key characteristic
  Serial.println("Subscribing to characteristic ...");
  if (!characteristic) {
    Serial.println("no characteristic found!");
    peripheral.disconnect();
    return;
  } else if (!characteristic.canSubscribe()) {
    Serial.println("characteristic is not subscribable!");
    peripheral.disconnect();
    return;
  } else if (!characteristic.subscribe()) {
    Serial.println("subscription failed!");
    peripheral.disconnect();
    return;
  } else {
    Serial.println("Subscribed");
  }
  int time=0;
  uint8_t haveData = 0;
  while(!characteristic.valueUpdated()){
    delay(50);
    // time++;
  }
  if(time !=40){
    haveData = 1;
  }
  if(haveData){
    char str[20] = "";
    uint8_t* value = (uint8_t*)str;
    int numberofValue = characteristic.readValue(value,20);
    Temperature = (char*)value;
  }
    Serial.println(Temperature);
  Serial.println("Disconnected");
}

static void setup_wifi()                                        //ham thiet lap ket noi wifi
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);                            //ket noi wifi theo phuong thuc WiFi
  while (WiFi.status() != WL_CONNECTED)                  //loading wifi
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());                       //ghi dia chi wifi len man hinh monitor
}

static void callback(char * topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0'; // Đảm bảo kết thúc chuỗi payload
  for(size_t i=0;i<length;i++){
    if((char)payload[i]>='0' && (char)payload[i]<='9'){
      message+= (char)payload[i];
    }
  }
  Serial.println(message);

  // Kiểm tra nếu thông điệp là phản hồi thuộc tính
  if (String(topic) == "v1/devices/me/rpc/response/1") {
    maxTemp = message.toInt();
    Serial.print("Max Temp: ");
    Serial.println(maxTemp);
  }else if (String(topic) == "v1/devices/me/rpc/response/2") {
    minTemp = message.toInt();
    Serial.print("Min Temp: ");
    Serial.println(minTemp);
  }
  message = "";
}

static void connectToBroker(void)
{
  while(!mqtt_client.connected()){
    Serial.println("Connecting MQTT ...");
    if(mqtt_client.connect(MQTT_NAME, MQTT_USERNAME, MQTT_PASSWORD)){
      Serial.println("connected");
      // mqtt_client.subscribe(NODE_1);            //dang ki ban tin "node1" tu server
      // mqtt_client.subscribe(NODE_2);            //dang ki ban tin "node2" tu server
      // mqtt_client.subscribe(NODE_3);            //dang ki ban tin "node3" tu server
      // mqtt_client.subscribe("v1/devices/me/attributes/response/1");
      char* request1 = "{\"method\": \"getMaxTemp\",\"params\": {}}";
      mqtt_client.publish("v1/devices/me/rpc/request/1" , request1);
      char* request2 = "{\"method\": \"getMinTemp\",\"params\": {}}";
      mqtt_client.publish("v1/devices/me/rpc/request/2" , request2);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());                //neu mat ket noi se gui ve cua so monitor trang thai hien tại của client
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}