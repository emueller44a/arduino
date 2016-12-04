#include <SPI.h>
//#include <Dhcp.h>
#include <Ethernet.h>
//#include <Dns.h>
//#include <EthernetServer.h>
//#include <util.h>
#include <EthernetUdp.h>
//#include <EthernetClient.h>
#include <EEPROM.h>

/*
 * keyboard firmware
 *
 * author: muellere
 */
const char version[] = "0.12";
const char date[] = "20161204"; 

 /*
  * packet format
  * 
  * WRITE_CMD
  * 2 bytes length
  * 0x01 <val>
  * value of variable 'val' will be used calling Keyboard.write(<val>)
  * 
  * PRESS_CMD
  * 2 bytes length
  * 0x02 <val>
  * value of variable 'val' will be used calling Keyboard.write(<val>)
  * 
  * RELEASE_ALL_CMD
  * 1 byte length
  * 0x03 <val>
  * calling Keyboard.releaseAll();
  * 
  * CONFIG_CMD
  * 2 bytes length
  * 0xFF <val>
  * value of variable 'val' containing new ip address will be stored in EEPROM 
  */



const boolean debug = true;
const boolean mute = true;
const boolean greeting = true;
const boolean enable_config_cmd = true;

const byte WRITE_CMD = 0x01;
const byte PRESS_CMD = 0x02;
const byte RELEASE_ALL_CMD = 0x03;
const byte CONFIG_CMD = 0xFF;

unsigned int local_port = 8888; 
const int eeprom_addr = 0;
byte magic_byte;

// threshold is 60 seconds
const unsigned long config_threshold = 60;
unsigned long config_timestamp;

EthernetUDP Udp;
char packet_buffer[UDP_TX_PACKET_MAX_SIZE];
char packet_content[UDP_TX_PACKET_MAX_SIZE];

void setup() {

  config_timestamp = uptime();
  
  Serial.begin(9600);
  
  if (debug) {
    // hold until serial console is connected
    while (!Serial) {}
    Serial.println("VirtualKeyboard firmware");
    Serial.print("version: ");
    Serial.println(version);
    Serial.print("date: ");
    Serial.println(date);
    Serial.print("enable_config_cmd: ");
    Serial.println(enable_config_cmd);
    Serial.println();
  }
  
  magic_byte = EEPROM.read(eeprom_addr);

  if (debug) {
    Serial.print("magic byte is: ");
    Serial.println(magic_byte);
  }

  if (debug) {
    Serial.print("UDP_TX_PACKET_MAX_SIZE is: ");
    Serial.println(UDP_TX_PACKET_MAX_SIZE);
  }
  
  /* 
   *  Calculating derived ip and mac adress
   *  1 -> DE-AD-BE-EF-FE-01, 192.168.2.1 
   *  2 -> DE-AD-BE-EF-FE-02, 192.168.2.2
   *  ...
   */
  
  byte ip_end = magic_byte;
  
  if (debug) {
    Serial.print("ip address is: 192.168.2.");
    Serial.println(ip_end);
    Serial.print("mac address is: DE-AD-BE-EF-FE-");
    char pp_ip_end[2];
    sprintf(pp_ip_end, "%02X", ip_end);
    Serial.println(pp_ip_end);
  }

  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, ip_end};
  
  IPAddress ip(192, 168, 2, ip_end);
  
  Keyboard.begin();
  Ethernet.begin(mac,ip);
  
  Udp.begin(local_port);
  
  if (debug) {
    Serial.println("waiting 5 seconds");
  }

  // wait 5 seconds
  delay(5000);

  if (debug) {
    Serial.println("dumping firmware info via keyboard");
  }

  if (greeting) {
    //EEPROM.write(eeprom_addr,42);
   
    const char HP355_COLON = 0x3E;
    //KEY_RETURN, 0xB0, 176
    const char HP355_RETURN = 0xB0;
    const char HP355_HYPHEN = 0x2F;
  
    // character 'z' is generating keyboard event 'y'
    Keyboard.println("kezboard firmware\0");
    Keyboard.write(HP355_RETURN);
    delay(10);
    Keyboard.print("version\0");
    Keyboard.write(HP355_COLON);
    Keyboard.write(' ');
    Keyboard.print(version);
    Keyboard.write(HP355_RETURN);
    delay(10);
    Keyboard.print("date\0");
    Keyboard.write(HP355_COLON);
    Keyboard.write(' ');
    Keyboard.print(date);
    Keyboard.write(HP355_RETURN);
    delay(10);
    // character '?' is generating keyboard event '_'
    Keyboard.print("enable?config?cmd\0");
    Keyboard.write(HP355_COLON);
    Keyboard.write(' ');
    if (enable_config_cmd) {
      Keyboard.print("true");
    } else {
      Keyboard.print("false");
    }
    Keyboard.write(HP355_RETURN);
    delay(10);
    Keyboard.print("ip\0");
    Keyboard.write(HP355_COLON);
    Keyboard.print(" 10.176.18.\0");
    Keyboard.print(ip_end);
    Keyboard.write(HP355_RETURN);
    delay(10);
    Keyboard.print("mac\0");
    Keyboard.write(HP355_COLON);
    Keyboard.print(" DE\0");
    Keyboard.write(HP355_HYPHEN);
    Keyboard.print("AD\0");
    Keyboard.write(HP355_HYPHEN);
    Keyboard.print("BE\0");
    Keyboard.write(HP355_HYPHEN);
    Keyboard.print("EF\0");
    Keyboard.write(HP355_HYPHEN);
    Keyboard.print("FE\0");
    Keyboard.write(HP355_HYPHEN);
    {
      char pp_ip_end[2];
      sprintf(pp_ip_end, "%02X", ip_end);
      Keyboard.print(pp_ip_end);
    }
    Keyboard.write(HP355_RETURN);
    delay(10);
  }
  
}


void loop() {

  int packet_size = read_udp();
  if (packet_size != 0) {
    switch (byte(packet_buffer[0])) {
      case WRITE_CMD:
        if (debug) {
          dump_ts_serial();
          Serial.print(" ");
          Serial.println("processing WRITE_CMD type packet");
        }
        if (!mute) {
          Keyboard.write(packet_buffer[1]);
        }
        break;
      case PRESS_CMD:
        if (debug) {
          dump_ts_serial();
          Serial.print(" ");
          Serial.println("processing PRESS_CMD type packet");    
        }
        if (!mute) {
          Keyboard.press(packet_buffer[1]);
        }
        break;
      case RELEASE_ALL_CMD:
        if (debug) {
          dump_ts_serial();
          Serial.print(" ");
          Serial.println("processing RELEASE_ALL_CMD type packet");    
        }
        if (!mute) {
          Keyboard.releaseAll();
        }
        break;
      case CONFIG_CMD:
        if (enable_config_cmd) {
          if (uptime() > config_timestamp + 60) {
            if (debug) {
              dump_ts_serial();
              Serial.print(" ");
              Serial.println("processing CONFIG_TYPE type packet");
            }
            if (magic_byte == packet_buffer[1]) {
              dump_ts_serial();
              Serial.print(" ");
              Serial.println("processed value is identical with configured value");
            } else {
              dump_ts_serial();
              Serial.print(" ");
              Serial.println("writing value to eeprom");
              EEPROM.write(eeprom_addr,packet_buffer[1]);
              //Serial.println(packet_buffer[1], DEC);
            }
          } else {
            if (debug) {
              dump_ts_serial();
              Serial.print(" ");
              Serial.println("ignoring CONFIG_TYPE type packet since threshold is not reached");
            }
          }
          config_timestamp = uptime();
        } else {
          dump_ts_serial();
          Serial.print(" ");
          Serial.println("ignoring CONFIG_TYPE type packet since command is disabled");
        }
        break;
    }
  }
  delay(10);

}

int read_udp() {
  int packet_size = Udp.parsePacket();
  if (packet_size) {
    Udp.read(packet_buffer,UDP_TX_PACKET_MAX_SIZE);
    bool contains_0x00 = 0;
    for (int i=0; i < packet_size; i++) {
      if (packet_buffer[i] == 0x00) {
        i = packet_size;
        contains_0x00 = 1;
      }
    }
    if (debug) {
      //received packet '2F3G01..' [10.122.222.23, 8888, 4 bytes]
      dump_ts_serial();
      Serial.print(" ");
      Serial.print("received packet '");
      
      for (int i=0; i < packet_size; i++) {
        if (i!=0) {
          Serial.print(",");
        }
        char pp_byte[3];
        sprintf(pp_byte, "%02X", packet_buffer[i]);
        //Serial.print(pp_byte);
        //Serial.print(packet_buffer[i], HEX);
        Serial.print("yy");
      }
      Serial.print("' [");
      IPAddress remote_ip = Udp.remoteIP();
      for (int i =0; i < 4; i++) {
        Serial.print(remote_ip[i], DEC);
        if (i < 3) {
          Serial.print(".");
        }
      }
      Serial.print(", ");
      Serial.print(Udp.remotePort());
      Serial.print(", ");
      Serial.print(packet_size);
      Serial.println(" bytes]");
    }
    if (packet_size > 24 or packet_size == 0 or contains_0x00 == 1) {
      if (debug) {
        if (packet_size > 24) {
          dump_ts_serial();
          Serial.print(" ");
          Serial.println("ignoring packet since size is greater 24 bytes");
        }
        if (packet_size == 0) {
          dump_ts_serial();
          Serial.print(" ");
          Serial.println("ignoring packet since size is 0 byte");
        }  
        if (contains_0x00 == 1) {
          dump_ts_serial();
          Serial.print(" ");
          Serial.println("ignoring packet since content contains 0x00 byte");
        }
      }
      packet_size = 0;
    } else { 
      switch (byte(packet_buffer[0])) {
        case RELEASE_ALL_CMD:
          break;
        case PRESS_CMD:
        case WRITE_CMD:
        case CONFIG_CMD:
          if (packet_size < 2) {
            if (debug) {
              dump_ts_serial();
              Serial.print(" ");
              Serial.print("packet of type ");
              char pp_data[2];
              sprintf(pp_data, "%02X", packet_buffer[0]);
              Serial.println(" is incomplete");
            }
            packet_size = 0;
          }
          break;
        default:
          if (debug) {
            dump_ts_serial();
            Serial.print(" ");
            Serial.println("unknown packet type");
          }
          packet_size = 0;
          break;
      }
    }
    for (int i=1; i < packet_size; i++) {
      packet_content[i-1] = packet_buffer[i];
    }
    if (packet_size != 0) {
      packet_content[packet_size-1] = 0x00;
    }
    if (debug and packet_size != 0) {
      dump_ts_serial();
      Serial.print(" ");
      Serial.print("content is '");
      
      for (int i=0; i < packet_size; i++) {
        char pp_byte[2];
        sprintf(pp_byte, "%02X", packet_content[i]);
        Serial.print(pp_byte);
      }
      Serial.println("'");
    }
  }
  return packet_size;
}

void dump_ts_serial() {
  Serial.print("[");
  Serial.print(millis()*0.01, 3);
  Serial.print("]");
}

unsigned long uptime() {
  // returns uptime in seconds
  return millis() / 1000UL;
}


