String req = " ", payload = " ";

//char* data = "1#,1,416,850,979,922,516,111,111,102,100,115,444,555,555,555"; //slave 1
//char* data = "2#,2,777,400,220,450,100,150,224,785,900,120,640,520,130,880";  //slave 2

void calculateVolatage()
{
  float d0 = (analogRead(A0)) * (5.0 / 1023.0);
  //  Serial.print(d0);
  //  Serial.print("  ");

  float d1 = (analogRead(A1)) * (5.0 / 1023.0);
  //  Serial.print(d1);
  //  Serial.print("  ");

  float d2 = (analogRead(A2)) * (5.0 / 1023.0);
  //  Serial.print(d2);
  //  Serial.print("  ");

  float d3 = (analogRead(A3)) * (5.0 / 1023.0);
  //  Serial.print(d3);
  //  Serial.print("  ");

  float d4 = (analogRead(A4)) * (5.0 / 1023.0);
  //  Serial.println(d4);

  String zero = "0.0";
  //  payload = "\"d0\":" + String(d0) + ",\"d1\":" + String(d1) + ",\"d2\":" + String(d2) + ",\"d3\":" + String(d3) + ",\"d4\":" + String(d4) + ",\"d5\":" + zero;
  //  payload += ",\"d6\":" + zero + ",\"a0\":" + zero + ",\"a1\":" + zero + ",\"a2\":" + zero + ",\"a3\":" + zero + ",\"a4\":" + zero;
  //  payload += ",\"a5\":" + zero + ",\"f0\":" + zero + ",\"f1\":" + String(11);
  //  payload += "}";

  payload = String(d0) + "," + String(d1) + "," + String(d2) + "," + String(d3) + "," + String(d4) + "," + String(11);
  //return payload;
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600, SERIAL_8E1); //SERIAL_8E1
  pinMode(9, OUTPUT);
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(9, LOW);
  req.trim();

  while (Serial.available())
  {
    req = Serial.readString();
    //Serial.println(req);
  }

  if (req == "Slave_ID:01") {
    calculateVolatage();
    digitalWrite(9, HIGH);
     
    //Serial.println("REQ");
    //data.trim();
    Serial.flush();
    Serial.print(payload);
    Serial.flush();
    
    //Serial.print(data2);
    req = "";
    payload = "";
  }
}


