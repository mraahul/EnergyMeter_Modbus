# EnergyMeter_Modbus

There are two parts of this project as in motherboard(Master) and daughter board(Slave). Motherboard is embedded with GPS/GPRS to upload data retrieved from all the Energy meters and daughter boards in JSON format. All the Energy meters and daughter boards are connected with motherboard in daisy chain connection. This project has 2 Daughter boards and 5 Energy meters to test. Also, each daughter boards were assigned a device ID and string of hard coded data to simulate the data coming from sensors attached to the daughter boards. The data was uploaded via MQTT protocol and also there was an for file check. 

Let's say, if there was no network, the string was saved in ".txt" fromat in an SD card. And later when there was network again, the file was retrieved and data was uploaded again.
