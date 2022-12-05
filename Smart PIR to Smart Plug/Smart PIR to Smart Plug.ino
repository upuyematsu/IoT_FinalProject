#include <WiFi.h>
#include "KasaSmartPlug.h"
#include <Ethernet.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

/*
*     WIFI, MAC ADD, ECT. NOT ACTUAL PASSWORDS I CHANGED THEM TO UPLOAD TO GITHUB
*/

int led = 2;      // the pin that the LED is atteched to
int sensor = 4;   // the pin that the sensor is atteched to
int state = LOW;  // by default, no motion detected
int val = 0;      // variable to store the sensor status (value)
const char *ssid = "Default";
const char *password = "password";


KASAUtil kasaUtil;

// IP of the MySQL *server* here
IPAddress server_addr(0, 0, 0, 0);
// MySQL user login username
char user[] = "root";
// MySQL user login password
char password[] = "password";

EthernetClient client;
MySQL_Connection conn((Client *)&client);


void setup() {

  pinMode(led, OUTPUT);    // initalize LED as an output
  pinMode(sensor, INPUT);  // initialize sensor as an input

  // Database setup
  Ethernet.begin(mac_addr);
  Serial.println("Connecting...");
  if (conn.connect(server_addr, 1111, user, password)) {
    delay(1000);
  } else {
    Serial.println("Connection failed.");
  }

  int found;
  Serial.begin(115200);

  // connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(led, HIGH);  // turn LED ON to show error
  }
  Serial.println(" CONNECTED");

  found = kasaUtil.ScanDevices();
  Serial.printf("\r\n Found device = %d", found);

  // Print out devices name and ip address found..
  for (int i = 0; i < found; i++) {
    KASASmartPlug *p = kasaUtil.GetSmartPlugByIndex(i);
    Serial.printf("\r\n %d. %s IP: %s Relay: %d", i, p->alias, p->ip_address, p->state);
  }
  digitalWrite(led, LOW);  // turn LED OFF to show no more error
}

//Setting up vars for loop
unsigned long m;  //stands for milliseconds
int counter = 0;
int offCounter = 0;
int checkCounter = 0;
int lastLoopIfLightsOff = 0;

void loop() {

  KASASmartPlug *testPlug = kasaUtil.GetSmartPlug("Lights");
  val = digitalRead(sensor);  // read sensor value

  // Check to see if lights are on and send to database
  if (checkCounter > 600) {
    int isOn = testPlug->GetRelayState();
    //send to database
    // Initiate the query class instance
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    // Execute the query
    cur_mem->execute(isOn);
    // Note: since there are no results, we do not need to read any data
    // Deleting the cursor also frees up memory used
    delete cur_mem;
  }

  if (val == HIGH) {  // check if the sensor is HIGH

    counter = 0;  //reset counter

    //millis tells how many milliseconds have passed since the program has started
    //It loops back to zero after aroound 50 days
    m = millis();
    unsigned long seconds = m / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    // To have lights stay off from 12 am to 7 am, plug in ESP32 at 6 pm
    //I don't want to wakeup in the middle of the night
    if (hours % 24 == 6 || hours % 24 == 7 || hours % 24 == 8 || hours % 24 == 9 || hours % 24 == 10 || hours % 24 == 11 || hours % 24 == 12 || hours % 24 == 13) {
      int x = 3;
      Serial.println("hours should not turn on light: ");
      Serial.println(hours);
    } else {

      testPlug->SetRelayState(1);
      delay(200);  // delay 100 milliseconds

      if (state == LOW) {
        Serial.println("Motion detected!");
        state = HIGH;  // update variable state to HIGH
      }
      Serial.println("hours, turning on the lights: ");
      Serial.println(hours);
    }
  } else {
    if (counter > 1800) {  // turn off after 30 mins
      //turn lights off
      testPlug->SetRelayState(0);
    }
    counter++;
  }

  delay(1000);
}