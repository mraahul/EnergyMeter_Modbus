#define TINY_GSM_MODEM_SIM800
#define SerialAT Serial
#define RE A9
#define MODBUS_TIMEOUT        8000
#define MODBUS_QUERY_INTERVAL 1000
#define NO_OF_DB 2
#define SD_CS 53

#include <avr/wdt.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <DS3231.h>
#include <SD.h>
#include <SPI.h>

typedef char* (*Func_Handler)(void);

int address[6] = {3002, 3018, 3034, 3006, 3022, 3038};
int did[5] = {1, 2, 3, 4, 5};

boolean payload_flag = false;
boolean datasend_flag = true;
boolean serialEventflag = true;

// GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "www";
const char user[] = "";
const char pass[] = "";

//const char* broker = "34.210.102.154"; //"13.58.161.1";
const char* broker = "mqttserver.com";
const char* user1 = "nT5JvOgANUtTMqzDrd";
const char* pass1 = "JGTtyrpE84fZRqAZlc";
int port = 1883; //13582;

unsigned long startindex = 1, lastindex = 0, diff = 0, strindex = 0;
unsigned long lastReconnectAttempt = 0;
const char* topic = "GsmClientTest/init";

unsigned long previoustime = 0, dataFetchInterval = 1000, dataUploadInterval = 0;

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
DS3231 clock;
RTCDateTime dt;
File myFile;

String payload = "";
String x = "";
String upload_data = "";
String ts = "0";
String rec = "";

int  i = 0, err = 0;

char data[250];

//char *dbid[NO_OF_DB] = {"\nSlave_ID:01", "\nSlave_ID:01", "\nSlave_ID:02"};
unsigned long currentmillis = 0;
int func_count = 0, DB_enable = 0, DB_count = 0, did_count = 1300;
//char *slave_id = " ";

boolean mqttConnect() {
  Serial.print(F("Connecting to "));
  Serial.print(broker);
  if (!mqtt.connect("prometheanMB1", user1, pass1, topic, 0, 1, "promethean")) {
 // if (!mqtt.connect("GsmClientTest")) {
    Serial.println(F(" fail"));
    return false;
  }
  Serial.println(F(" OK"));
  mqtt.subscribe(topic);
  return mqtt.connected();
}

void if_connected() {
  if (mqtt.connected()) {
    mqtt.loop();
  }
  else {
    // Reconnect every 10 seconds
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }
}

void sd_init() {

  String _temp = " ";
  // Serial.print(F("Initializing SD card..... "));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Initialization Failed!"));
    while (1);
  }
  Serial.println(F("SD Initialization Done."));

  //SD.end();

  /*****************************************************************/
  /*____________________Read Start Filename________________________*/
  /*****************************************************************/

  if (SD.exists("START.TXT")) {
    //Serial.println(F("Reading From Start.txt ....."));
    myFile = SD.open("START.TXT");
    if (myFile) {
      while (myFile.available()) {
        _temp = myFile.readString();
      }
      myFile.close();
      startindex = _temp.toInt();
    }
  } else {
    //Serial.println(F("Creating Start.txt ......."));
    myFile = SD.open("START.TXT", FILE_WRITE);
    if (myFile) {
      myFile.print("1");
      myFile.close();
      startindex = 1;
    }
  }

  /*****************************************************************/
  /*____________________Read End Filename________________________*/
  /*****************************************************************/

  if (SD.exists("END.TXT")) {
    //Serial.println(F("Reading From End.txt ....."));
    myFile = SD.open("END.TXT");
    if (myFile) {
      while (myFile.available()) {
        // Serial.write(myFile.read());
        _temp = myFile.readString();
      }
      myFile.close();
      lastindex = _temp.toInt();
    }
  } else {
    //Serial.println(F("Creating End.txt ......."));
    myFile = SD.open("END.TXT", FILE_WRITE);
    if (myFile) {
      myFile.print("0");
      myFile.close();
      lastindex = 0;
    }
  }

  diff = lastindex - startindex + 1;

  /*****************************************************************/
  /*___________________Read Last String Index______________________*/
  /*****************************************************************/

  //  if (lastindex > 0) {
  //    _temp = String(lastindex) + ".TXT";
  //    myFile = SD.open(_temp);
  //    if (myFile) {
  //      while (myFile.available()) {
  //        _temp = myFile.readString();
  //      }
  //      Serial.println(_temp);
  //      myFile.close();
  //    } else {
  //      Serial.println(F("Error Opening File...!!!"));
  //    }
  //
  //    _temp.trim();
  //    strindex = (_temp .substring((_temp.length()) - 1)).toInt();
  //  }
  //
  //  Serial.print(F("Start Index = "));
  //  Serial.println(startindex);
  //  Serial.print(F("Last Index = "));
  //  Serial.println(lastindex);
  //  Serial.print(F("String Index = "));
  //  Serial.println(strindex);

  myFile.flush();

  Serial.print("Start = ");
  Serial.println(startindex);
  Serial.print("last = ");
  Serial.println(lastindex);

  /**************END FUNCTION********************/
}

boolean file_chk(String str) {
  String _temp = " ";
  int alpha = 0, quotes = 0, colon = 0, brackets = 0, comma = 0, num = 0, len = 0;
  int alpha_count = 20, quotes_count = 34, colon_count = 17, comma_count = 16, num_count = 72;
  int a = 0, b = 1, temp = 0, lower_limit = 155, upper_limit = 180;
  len = str.length();       //DB:1 EM:100

  Serial.print("\nChecking payload...\nLength :");
  Serial.println(len);

  char dat[len];
  boolean _fc_flag = false;

  int i0 = 0, i1 = 0;
  String val = "";

  i0 = payload.indexOf('"', 2);
  val = payload.substring(2, i0);

  //  Serial.println("");
  //  Serial.print("Alpha = ");
  //  Serial.print(alpha_count);
  //  Serial.print("  Quotes = ");
  //  Serial.print(quotes_count);
  //  Serial.print("  Colon = ");
  //  Serial.print(colon_count);
  //  Serial.print("  Comma = ");
  //  Serial.print(comma_count);
  //  Serial.print("  Number = ");
  //  Serial.print(num_count);
  //  Serial.println("");


  /****************************************************/
  /*================ File Size Check =================*/
  /****************************************************/

  if (str.length() < lower_limit || str.length() > upper_limit) {
    Serial.println(F("File Size Mismatch"));
    _fc_flag = false;
  }
  else {
    _fc_flag = true;
    Serial.println(F("File Size OK"));
  }

  /****************************************************/
  /*=============== Characters Check =================*/
  /****************************************************/
  str.toCharArray(dat, len);
  if (_fc_flag == true) {
    for (int k = 0; k < len; k++) {
      if (isDigit(dat[k])) {
        num++;
      }
      else if (isAlpha(dat[k])) {
        alpha++;
      }
      else if (isAscii(dat[k])) {
        if (dat[k] == '"') {
          quotes++;
        }
        else if (dat[k] == ':') {
          colon++;
        }
        else if ((dat[k] == '{') || (dat[k] == '}')) {
          brackets++;
        }
        else if (dat[k] == ',') {
          comma++;
        }
      }
    }

    if (alpha == alpha_count && quotes == quotes_count && colon == colon_count && comma == comma_count && brackets == 1) {
      Serial.println(F("Character Check OK"));
      _fc_flag = true;
    }
    else {
      _fc_flag = false;
      Serial.println(F("Error in Char check"));
    }
  }

  //  Serial.println("");
  //  Serial.print("Alpha = ");
  //  Serial.print(alpha);
  //  Serial.print("  Quotes = ");
  //  Serial.print(quotes);
  //  Serial.print("  Colon = ");
  //  Serial.print(colon);
  //  Serial.print("  Comma = ");
  //  Serial.print(comma);
  //  Serial.print("  Number = ");
  //  Serial.print(num);
  //  Serial.print("  Brackets = ");
  //  Serial.print(brackets);
  //  Serial.println("");

  /****************************************************/
  /*================== Data Check ====================*/
  /****************************************************/

  str.toCharArray(dat, len);
  if (_fc_flag == true) {
    for (int k = 0; k < colon_count - 1  ; k++) {
      while (dat[a] != ':') {
        a++;
        if (a > len) {
          break;
        }
      }
      temp = a;

      while (dat[b] != ',') {
        b++;
        if (b > len) {
          break;
        }
      }

      for (int j = a + 1; j < b; j++) {
        if (isAlpha(dat[j])) {
          Serial.println(F("Filter 3 test failed_1, Delete File"));
          _fc_flag = false;
          break;
        }
        else if ((isDigit(dat[j])) || (isAscii(dat[j] == '.'))) {
          //Serial.println("Digit");
        }
        else {
          _fc_flag = false;
          Serial.println(F("Filter 3 test failed_2, Delete File"));
          break;
        }
        // Serial.print(dat[j]);
      }
      //Serial.println(" ");
      b++;
      a++;
      if (_fc_flag == false) {
        _fc_flag = false;
        break;
      }
    }
  }
  /****************************************************/
  /*================== Return Flag ===================*/
  /****************************************************/

  if (_fc_flag == false) {
    Serial.println("Returning false");
    Serial.println(payload);
    Serial.println("");
    return false;
  }
  else if (_fc_flag == true) {
    Serial.println("Returning true");
    Serial.println(payload);
    Serial.println("");
    return true;
  }
}

String modbus_query(char *slave_id)
{
  /*if (slave_id == "Slave_ID:00")
    {
      func_count++;
      slave_id = "Slave_ID:11";
    }
    //Serial.print(" Q-id : ");
    Serial.print(slave_id);*/
  digitalWrite(RE, HIGH);
  Serial1.print(slave_id);
  //Serial.println();
  delay(100);
  digitalWrite(RE, LOW);
  //  digitalWrite(DE, LOW);
  return slave_id;
}


unsigned int ModRTU_CRC(unsigned char buf[], int len)
{
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++)
  {
    crc ^= (unsigned int)buf[pos];          // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  //crc1 = crc >> 8;
  //crc = (crc << 8) | crc1;
  //crc &= 0xFFFF;
  return crc;
}

uint8_t packet[256];
uint8_t response1[3], response3[2];
byte response2[4] = {0, 0, 0, 0};
byte response[9];

void generatePacket(unsigned int address, unsigned int bytesToRead, unsigned int did) {
  packet[0] = did;
  packet[1] = 03;
  packet[2] = highByte(address);
  packet[3] = lowByte(address);
  packet[4] = highByte(bytesToRead); //High byte
  packet[5] = lowByte(bytesToRead); //Low byte

  unsigned int crc = ModRTU_CRC(packet, 6);
  packet[6] = lowByte(crc);
  packet[7] = highByte(crc);
}

float get_data_float32(unsigned int addr, unsigned int did)
{
  generatePacket(addr, 2, did);
  unsigned long time_limit = millis();
  char dump1;
  while (Serial1.available() && ((millis() - time_limit) < 500))
  {
    dump1 = Serial1.read();
  }

  digitalWrite(RE, HIGH);
  for (int i = 0; i < 8; i++)
    Serial1.write(packet[i]);
  delayMicroseconds(1500); //2000
  Serial1.flush();
  delayMicroseconds(3000);  //3437
  digitalWrite(RE, LOW);
  delay(100);

  int gflag1 = 0;
  if (Serial1.available()) // is there something to check?
  {
    Serial1.readBytes(response, 9);
    delayMicroseconds(1500); // inter character time out  //1718
    unsigned int crc1 = ModRTU_CRC(response, 7);

    if (!((response[7] == lowByte(crc1)) && (response[8] == highByte(crc1))))
    {
      response[3] = 0;
      response[4] = 0;
      response[5] = 0;
      response[6] = 0;
      gflag1 = 3;
    }
  }
  else
  {
    response[3] = 0;
    response[4] = 0;
    response[5] = 0;
    response[6] = 0;
    gflag1 = 4;
  }

  String W1 = String(response[3], BIN);
  String W2 = String(response[4], BIN);
  String W3 = String(response[5], BIN);
  String W4 = String(response[6], BIN);

  W1 = checkdigit(W1);
  W2 = checkdigit(W2);
  W3 = checkdigit(W3);
  W4 = checkdigit(W4);

  String W = W1 + W2 + W3 + W4;
  float Wf = converttofloat(W);
  if (gflag1)
  {
    return gflag1;
  }
  else
  {
    return Wf;
  }
}

String checkdigit(String x)
{
  int i = 8 - x.length();
  if (i > 0)
  {
    while (i > 0)
    {
      x = "0" + x;
      i--;
    }
  }
  return x;
}

float converttofloat(String S)
{
  float temp = 0;
  if (S[0] == '1')
    temp = 1;
  float sign = pow(-1, temp);
  float e = 0;
  for (int i = 8, j = 0; i > 0; j++, i--)
  {
    float temp = 0;
    if (S[i] == '1')
      temp = 1;
    e = e + (pow(2, j) * temp);
  }
  float temp1 = pow(2, (e - 127));
  float temp2 = 0;
  for (int i = 31, j = 23; i > 8; i--, j--)
  {
    float temp = 0;
    if (S[i] == '1')
      temp = 1;
    temp2 = temp2 + (1 / pow(2, j)) * temp;
  }
  temp2 = temp2 + 1;
  float result = sign * temp1 * temp2;
  return result;
}

String get_timestamp() {

  String ts = "";
  dt = clock.getDateTime();

  //ts = clock.dateFormat("d-m-Y H:i:s", dt);
  ts = clock.dateFormat("U", dt);
  ts = String(ts.toInt() + 3600);

  return ts;
}

String get_payload() {
  String payload = "", _temp = "", zero = "0.0";
  int i0 = 0, i1 = 0, i2 = 0, lvl = 0, csq = 0;

  float voltage = (analogRead(A10)) * (5.0 / 1023.0);
  payload = "{\"dId\":" + String(did_count);

  ts = get_timestamp();
  payload += ",\"ts\":" + String(ts);

  i0 = x.indexOf(',', 0);
  _temp = x.substring(0, i0);
  payload += ",\"d0\":" + _temp;

  i1 = x.indexOf(',', 0);
  i2 = x.indexOf(',', i0 + 1);
  _temp = x.substring(i1 + 1, i2);
  payload += ",\"d1\":" + _temp;

  i2 = x.indexOf(',', i1 + 1);
  i1 = x.indexOf(',', i2 + 1);
  _temp = x.substring(i2 + 1, i1);
  payload += ",\"d2\":" + _temp;

  i1 = x.indexOf(',', i2 + 1);
  i2 = x.indexOf(',', i1 + 1);
  _temp = x.substring(i1 + 1, i2);
  payload += ",\"d3\":" + _temp;

  i2 = x.indexOf(',', i1 + 1);
  i1 = x.indexOf(',', i2 + 1);
  _temp = x.substring(i2 + 1, i1);
  payload += ",\"d4\":" + _temp;

  i1 = x.indexOf(',', i2 + 1);
  i2 = x.indexOf(',', i1 + 1);
  _temp = x.substring(i1 + 1, i2);
  payload += ",\"d5\":" + _temp;

  payload += ",\"d6\":" + zero + ",\"a0\":" + zero + ",\"a1\":" + zero + ",\"a2\":" + zero + ",\"a3\":" + zero + ",\"a4\":" + zero;
  payload += ",\"a5\":" + zero + ",\"f0\":" + zero + ",\"f1\":" + String(voltage * 2);
  payload += "}";

  return payload;
}

void publishData()
{
  String _temp = "";
  if (payload_flag == true) {   // Send Current Payload
    payload_flag = false;

    // if_connected();

    // payload = get_payload();

    if (mqtt.publish(topic, payload.c_str(), 1))
    {
      //"{\"did\":2,\"ts\":20180308004520,\"a0\":777,\"a1\":400,\"a2\":220,\"a3\":450,\"a4\":100,\"a5\":150,\"a6\":224,\"a7\":785,\"a8\":900,\"a9\":120,\"a10\":640,\"a11\":520,\"a12\":130,\"a13\":880,\"lvl\":435,\"sig\":12}"
      Serial.println("");
      Serial.println("");
      Serial.println(F("Published succesfully\n\n"));
      Serial.println("");
      err = 0;
    }
    else
    {
      Serial.println("");
      Serial.println(F("Publish Failed\n\n"));
      Serial.println("");

      lastindex++;
      err++;
      if (err >= 7) {
        digitalWrite(A3, LOW);
        delay(5000);
        digitalWrite(A3, HIGH);
        delay(500);

        //digitalWrite(A3, HIGH);
        digitalWrite(A3, LOW);
        delay(1000);
        digitalWrite(A3, HIGH);
        modem.restart();
        modem.waitForNetwork();
        modem.gprsConnect(apn, user, pass);
        mqttConnect();
        err = 0;
      }
      payload_flag = false;

      _temp = String(lastindex) + ".txt";
      myFile = SD.open(_temp, FILE_WRITE);
      if (myFile)
      {
        myFile.print(payload);
      }
      myFile.close();
      myFile.flush();

      if (SD.exists("END.TXT"))
      {
        SD.remove("END.TXT");
      }

      myFile = SD.open("END.TXT", FILE_WRITE);
      if (myFile) {
        //Serial.println(F("Re-writing to Start.txt....."));
        myFile.println(String(lastindex));
        myFile.close();
        myFile.flush();
        //Serial.println(F("Updated Start.txt....."));
      } else {
        //Serial.println(F("Error Opening Start.txt......"));
      }
    }
  }
  else if (payload_flag == false) {   //Send data from sd card

    // if_connected();

    if (lastindex > 0) {
      _temp = String(startindex) + ".TXT";
      if (SD.exists(_temp)) {
        myFile = SD.open(_temp);
        Serial.println(_temp);
        if (myFile) {
          while (myFile.available()) {
            payload = myFile.readString();
          }
          myFile.close();
        }

        if (file_chk(payload)) {
          if (mqtt.publish(topic, payload.c_str(), 1)) {      // Past Data.
            SD.remove(_temp);
            startindex++;
            Serial.println("");
            Serial.println("");
            Serial.println(F("SD Data Published succesfully\n\n"));
            Serial.println("");
          }
        }
        else {
          SD.remove(_temp);
          startindex++;
          Serial.println("");
          Serial.println("");
          Serial.println("SD Data Publish Failed\n\n");
          Serial.println("");
        }
      }
      else {
        startindex++;
      }


      if (SD.exists("START.TXT")) {
        SD.remove("START.TXT");
      }

      myFile = SD.open("START.TXT", FILE_WRITE);
      if (myFile) {
        //Serial.println(F("Re-writing to Start.txt....."));
        myFile.println(String(startindex));
        myFile.close();
        myFile.flush();
        //Serial.println(F("Updated Start.txt....."));
      }
      // }
    }
  }

  diff = lastindex - startindex + 1;
  if (diff <= 0) {
    // if_connected();
    lastindex = 0;
    startindex = 1;
    diff = 0;
    if (SD.exists("START.TXT")) {
      SD.remove("START.TXT");
    }

    myFile = SD.open("START.TXT", FILE_WRITE);
    if (myFile) {
      //Serial.println(F("Re-writing to Start.txt....."));
      myFile.println(String(startindex));
      myFile.close();
      myFile.flush();
      //Serial.println(F("Updated Start.txt....."));
    }

    if (SD.exists("END.TXT")) {
      SD.remove("END.TXT");
    }

    myFile = SD.open("END.TXT", FILE_WRITE);
    if (myFile) {
      //Serial.println(F("Re-writing to Start.txt....."));
      myFile.println(String(lastindex));
      myFile.close();
      myFile.flush();
      //Serial.println(F("Updated Start.txt....."));
    } else {
      //Serial.println(F("Error Opening Start.txt......"));
    }
  }

  //  if (resetmillis + 300000 < millis()) {
  //    modem.restart();
  //    modem.waitForNetwork();
  //    modem.gprsConnect(apn, user, pass);
  //    mqttConnect();
  //    resetmillis = millis();
  //  }

  Serial.print(F("Start = "));
  Serial.println(startindex);
  Serial.print(F("last = "));
  Serial.println(lastindex);
  Serial.print(F("Diff = "));
  Serial.println(diff);
  Serial.print(F("ERR = "));
  Serial.println(err);

  Serial.flush();
  Serial1.flush();
  myFile.flush();
}

void read_all_DB()
{
  static int func_count = 0;
  if ((currentmillis + MODBUS_QUERY_INTERVAL < millis() && serialEventflag == true) || (currentmillis + MODBUS_TIMEOUT < millis()))
  {
    serialEventflag = false;

    //      Serial.print("B4: ");
    //        Serial.print(func_count);
    //       Serial.print(" B4_DB: ");
    //        Serial.println(DB_count);
    //       Serial.println("3");

    //slave_id = (Daughter_Board[func_count])();

    if ( func_count == 0) {
      //modbus_query("Slave_ID:01");
      //Serial.print("A4: Slave_1");
      func_count++;
      DB_count++;

    }
    else if ( func_count == 1) {
      Serial.println();
      //Serial.print("A4: Slave_2");
      modbus_query("Slave_ID:01");
      func_count++;
      DB_count++;

    }
    else if ( func_count >= 2 && func_count <= 4) {
      //Serial.print("A4: Slave_3");
      modbus_query("Slave_ID:02");
      func_count++;
      DB_count++;

    }
    else { //( func_count == 2) {
      //Serial.print("A4: Slave_4");
      //modbus_query("Slave_ID:01");
      func_count = 0;
      DB_enable = 0;
      datasend_flag = true;
      DB_count = 0;
    }


    //    Serial.print("A4: ");
    //    Serial.print(func_count);
    //    //Serial.print(" A4_DB: ");
    //    Serial.println(DB_count);

    // slave_id = dbid[func_count];
    /*if (func_count == 0)
      {

      //Serial.println("Hiii");
      func_count++;
      }
      slave_id = dbid[func_count];

      /*if (func_count == 0)
      {
      func_count++;
      slave_id = dbid[func_count];
      //slave_id = "Slave_ID:01";
      }
      //Serial.print(" Q-id : ");
      Serial.println(slave_id);
    */
    //  modbus_query(slave_id);
    //  Serial.print("inloop: ");
    //Serial.println(slave_id);

    currentmillis = millis();
    // delay(100);
  }

  while (Serial1.available())
  {
    x = Serial1.readString();
    did_count++;
    payload = get_payload();
    Serial.println(payload);

    payload_flag = true;
    publishData();

    func_count++;
    currentmillis = millis();

    Serial1.flush();
    Serial.flush();

    //    if (func_count > NO_OF_DB - 1)
    //    {
    //      func_count = 0;
    //      DB_count = 0;
    //      DB_enable = 0;
    //      datasend_flag = true;
    //    }
  }

  digitalWrite(RE, LOW);
  serialEventflag = true;

  Serial1.flush();
  Serial.flush();
}

void read_EM()
{
  did_count = 1300;
  for (int i = 0; i < 5; i++)
  {
    did_count++;
    //    Serial.print("Reading EM_");
    //    Serial.print(did[i]);
    //Serial.print("\nW1 = ");
    float Watt_1 = (get_data_float32(address[0], did[i]));
    //Serial.print(Watt_1);

    //Serial.print("  W2 = ");
    float Watt_2 = (get_data_float32(address[1], did[i]));
    //    Serial.print(Watt_2);
    //
    //    Serial.print("  W3 = ");
    float Watt_3 = (get_data_float32(address[2], did[i]));
    //    Serial.print(Watt_3);
    //
    //    Serial.print("  PF_1  = ");
    float PF_1 = get_data_float32(address[3], did[i]);
    //    Serial.print(PF_1);
    //
    //    Serial.print("  PF_2  = ");
    float PF_2 = get_data_float32(address[4], did[i]);
    //    Serial.print(PF_2);
    //
    //    Serial.print("  PF_3  = ");
    float PF_3 = get_data_float32(address[5], did[i]);
    //Serial.println(PF_3);

    String zero = "0.0";
    ts = get_timestamp();
    float voltage = (analogRead(A10)) * (5.0 / 1023.0);

    upload_data = "{\"dId\":" + String(did_count); upload_data += ",\"ts\":" + String(ts);
    upload_data += ",\"d0\":" + String(Watt_1);    upload_data += ",\"d1\":" + String(Watt_2);  upload_data += ",\"d2\":" + String(Watt_3);
    upload_data += ",\"d3\":" + String(PF_1);  upload_data += ",\"d4\":" + String(PF_2);   upload_data += ",\"d5\":" + String(PF_3);

    upload_data += ",\"d6\":" + zero + ",\"a0\":" + zero + ",\"a1\":" + zero + ",\"a2\":" + zero + ",\"a3\":" + zero + ",\"a4\":" + zero;
    upload_data += ",\"a5\":" + zero + ",\"f0\":" + zero + ",\"f1\":" + String(voltage * 2);
    upload_data += "}";
    //    Serial.print("upload_data");
    Serial.println(upload_data);

    payload = upload_data;
    payload_flag = true;

    publishData();
  }
}

void setup()
{
  Serial1.begin(9600, SERIAL_8E1);
  SerialAT.begin(9600/*, SERIAL_8E1*/);

  pinMode(RE, OUTPUT);
  pinMode(A1, OUTPUT);

  digitalWrite(RE, LOW);
  digitalWrite(A1, LOW);
  Serial.println(F("Device Started."));
  
  clock.begin();
  Serial.println(F("RTC Initialization Done."));

//  if (!SD.begin(SD_CS)) {
//    Serial.println(F("Initialization Failed!"));
//    while (1);
//  }
//  Serial.println(F("SD Initialization Done."));

    sd_init();
    modem.restart();
    modem.waitForNetwork();
    modem.gprsConnect(apn, user, pass);
    mqtt.setServer(broker, port);    //cloudmqtt
    //mqtt.setServer(broker, port);  //testmosquitto/promethean broker
    mqttConnect();
}

void loop()
{
  unsigned long currenttime = millis();
  if (currenttime - previoustime >= dataFetchInterval)
  {
    digitalWrite(A1, HIGH);

    //        Serial.print(F("\nDifference , CT, PT : "));
    //        Serial.print(currenttime - previoustime);
    //        Serial.print("  ");
    //        Serial.print(currenttime);
    //        Serial.print("  ");
    //        Serial.println(previoustime);

    while (DB_enable == 0 && datasend_flag == true)
    {
      Serial.println();
      read_EM();
      DB_enable = 1;
      datasend_flag = false;
      func_count = 0;
    }

    while (DB_enable == 1 && datasend_flag == false)
    {
      read_all_DB();
      //func_count = 0;
      //DB_enable = 0;
      //datasend_flag = true;
      //DB_count = 0;
    }
    previoustime = currenttime;
    dataFetchInterval = 60000;    //60000
  }
  digitalWrite(A1, LOW);
  //  Serial.println(".");
  publishData();
}
