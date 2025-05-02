/////////////////////////////////////////
// MPU6050 Gyroscope Interfacing with ESP32 | ESP32
// https://howtomechatronics.com/tutorials/arduino/arduino-and-mpu6050-accelerometer-and-gyroscope-tutorial/#h-mpu6050-orientation-tracking-3d-visualization
/////////////////////////////////////////

// Networkinng
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_MPU6050.h>
#include <Wire.h>


/////////////////////////////////////////
const unsigned long SERIAL_PORT_BAUD = 115200;

const char* UPLINK_IP = "192.168.xx.xx";        // IP address for the Data Daemon
const unsigned int UPLINK_UDP_SRV_PORT = 55555; // IP port for the Data Daemon 

const char* WLAN_SSID = "ssid";
const char* WLAN_PSW  = "password";

const unsigned int MCU_MPU_UDP_PORT = 13579;  

const int DATA_PKT_SN_STRAT = 1000;


/////////////////////////////////////////
// Global variables
/////////////////////////////////////////
WiFiUDP Udp;
Adafruit_MPU6050 Mpu;

unsigned int g_pck_sn;
uint32_t g_boot_time;


/////////////////////////////////////////
// Functions
/////////////////////////////////////////

/////////////////////////////////////////
// General
/////////////////////////////////////////
// Netowrking
void init_wifi(const char* aSsid, const char* aPwd){
  WiFi.begin(aSsid, aPwd);
  Serial.println("Begin: WiFi connection");
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
  }
  Serial.println("");
  Serial.println("End: WiFi Connected");
}

void init_udp(const int aPort){
  Udp.begin(aPort);
  Serial.printf("UDP listening at %s:%d\n", 
                WiFi.localIP().toString().c_str(), 
                aPort);
}

/* -----------------------
Text based protocal
- Header
1. UId: Device unit ID
2. PkgS: Package serial for UDP drop detection
3. GType: Datagram type, fixed as 'DM' for now
4. Tick: clock tick on device
- Data 
5. JSON string
- Example: UId:TestDeviceMPU|PkgS:13|GType:MPU|Tick:168888|{"AccX":-3.36, "AccY":-6.19, "AccZ":5.18}
 ----------------------- */
 String send_msg(const char * aIp, const int aPort, String aMsg) {
  String msg = "UId:TestDeviceMPU|PkgS:" + String(g_pck_sn) + "|" + aMsg;

  Udp.beginPacket(aIp, aPort);
  Udp.print(msg.c_str());
  Udp.endPacket();
 
  g_pck_sn++; 
  return(msg);
}


/////////////////////////////////////////
// Sensor
void init_mpu(){
  Serial.println("Begin: Find MPU");
  if (!Mpu.begin()) {
    Serial.println("End: Failed to find MPU");
    while (1) {
      delay(10);
      Serial.print(".");
    }
  }  
  Serial.println("END: Found MPU");
  
  Mpu.setAccelerometerRange(MPU6050_RANGE_8_G); 
  Mpu.setGyroRange(MPU6050_RANGE_500_DEG);      
  Mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   
  Serial.println("Accelerometer: +-8 G, Gyro: +-500 deg/s, Filter bandwidth: 21Hz");
}


String make_mpu_gram(){
  sensors_event_t a, g, temp;  
  Mpu.getEvent(&a, &g, &temp);

  return("GType:MPU|Tick:"+String(esp_timer_get_time())+
         "|{\"AccX\":" + String(a.acceleration.x)+ 
         ", \"AccY\":" + String(a.acceleration.y)+
         ", \"AccZ\":" + String(a.acceleration.z)+
         ", \"GyrX\":" + String(g.gyro.x)+
         ", \"GyrY\":" + String(g.gyro.y)+
         ", \"GyrZ\":" + String(g.gyro.z)+
         "}"         
        );
}


/////////////////////////////////////////
// Setup
/////////////////////////////////////////
void setup()
{  
  Serial.begin(SERIAL_PORT_BAUD);
  Serial.println("===== Begin of Set Up =====");

  init_wifi(WLAN_SSID, WLAN_PSW);
  init_udp(MCU_MPU_UDP_PORT);

  init_mpu();

  g_boot_time = esp_timer_get_time();
  g_pck_sn = 1000; 
  
  Serial.println("===== End of Set Up =====");
}


/////////////////////////////////////////
// Loop
/////////////////////////////////////////
void loop(){
  String msg = send_msg(UPLINK_IP, UPLINK_UDP_SRV_PORT, make_mpu_gram());  
  Serial.println(msg);

  delay(50);  
}
