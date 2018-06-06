#include <ESP8266WebServer.h>
#include <FS.h>AT


// To address the duck us http://192.168.4.1/stick and the joystick.html will be
// fetched.


//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiPWD[] = "";      // we are open, no password Wifi

/* The duck definition table. Each 8266 chips last 4 (right hand) hexadecimal
*  digits must be entered here in upper case without any 0's, i.e. 101 is 11.
*  this witht the setupWifi function uniquely assigns a SSID to the duck and
*  and the IP address will be 192.168.4.1.
*  
*  If you do not define a MAC in here for the 8266 device being run, the setupWifi 
*  function will configure the WIFI SSID as DUCKRACE_<MAC>, take the <MAC> and add 
*  to this table using whatever number you wish, as long as not already used.
*/
char *MAC_TABLE[10][2] = {
  {"35D", "1" },
  {"CD13", "2" },
  {"C78E", "3" },
  {"3D29", "4" },
  {"319F", "5" },
  {"C1E4", "6" },
  {"C21", "7" },
  {"C84E", "8" },
  {"3812", "9" },
  {"C286", "10" }
};
  
/////////////////////////////
// Compilation Definitions //
/////////////////////////////
#define DIO_HB_PWMLEFT 11
#define DIO_HB_EN1LEFT 12 
#define DIO_HB_EN2LEFT 13 
#define DIO_HB_PWMRIGHT 11
#define DIO_HB_EN1RIGHT 12 
#define DIO_HB_EN2RIGHT 13 

/////////////
// Globals //
/////////////
ESP8266WebServer server(80);    // define the server and listen on port 80 (standard)
unsigned char motor;            // last known motor power value
unsigned char rudder;           // last known direction value
const int RUDDER_INIT = 90;
long watchDogTimer=millis();    // our internal watchdog

/*
 * Status message to send back to the phone
 * this is for debugging purposes. The phone
 * just displays
 */
void sendStatus() {
  // create the response message
  String message="Setting power to ";
  message+=(int)motor;
  message+=" and rudder to ";
  message+=(int)rudder;
  server.send(200, "text/plain", message);
}

/*
 * Configures the features of the duck by
 * 1. Initalizing any special hardware (comms, GPIO's etc)
 * 2. Configuring the Wifi
 * 3. Initializing SPIFFS which contains the served HTML/JavaScript documents (uploaded separately)
 * 4. Defining the HTTP services and actions called as web services from phone processing
 */
void setup() 
{
  // initialize the operating environment
  initHardware();
  setupWiFi();

  // configure our on-chip filesystem and map service requests to file fetches
  SPIFFS.begin();

  // validate we have the correct content to run the duck - outputs
  // warning messages to serial port if not found
  if(SPIFFS.exists("/joystick.html")==false)
    Serial.println("joystick.html is missing from SPIFFS, please upload");

  if(SPIFFS.exists("/virtualjoystick.js")==false)
    Serial.println("virtualjoystick.js is missing from SPIFFS, please upload");

  // sets the relationship between we page requests and filesystem content
  server.serveStatic("/stick", SPIFFS, "/joystick.html");
  server.serveStatic("/virtualjoystick.js", SPIFFS, "/virtualjoystick.js");

  // set the power handlers for motor and rudder based on url argument values
  server.on("/duck", [](){
    int val;
    val=(unsigned char)server.arg("p").toInt();
    if(val!=NULL)
      motor=val;
    val=(unsigned char)server.arg("r").toInt();
    if(val!=NULL)
      rudder=val;

    // build the command string to send to the Arduino
    char command[]={ 0xfe, 0x07, (unsigned char)motor, (unsigned char)rudder, 0xfd };
    Serial.print(command);
    sendStatus();
  });

  // start the web server
  server.begin();
}

/*
 * Once everything configured, runs
 * this function continuously waiting
 * for HTTP requests and handling
 * based on the page handling instructions
 * previously configured.
 */
void loop() 
{
  // just handle the client requests, its that easy
  server.handleClient();
}

/*
 * Function to read the MAC address of the device
 * and map to a duck number. Sets the device as
 * an ACCESS POINT (like a Router) and configures
 * the SSID as DUCKRACE_<Num>
 */
void setupWiFi()
{
  // sets access point mode (works like a home router)
  WiFi.mode(WIFI_AP);

  // get the device MAC
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);

  // stringify and upper case it
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();

  // create the SSID name
  String AP_NameString = "DUCKRACE_"; // + macID;
  
  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, AP_NameString.length() + 1, 0);

  // read through the MAC_TABLE and find the configured MAC
  // address
  for (int i=0; i<(int)(sizeof(MAC_TABLE)/sizeof(MAC_TABLE[0]));i++) {
    // compare 
    if(strcmp(macID.c_str(),MAC_TABLE[i][0])==0) {
      Serial.println("Found MAC");
      AP_NameString+=MAC_TABLE[i][1];
      WiFi.softAP(AP_NameString.c_str(), WiFiPWD, i+1);       // i+1 sets the Wifi channel (each duck must be on separate channel)
      return;
    }
  }
  AP_NameString+=macID;
  WiFi.softAP(AP_NameString.c_str(), WiFiPWD);
}

void initHardware()
{
  // Initialize serial comms for debugging
  Serial.begin(57600);
  delay(250);
  
  // declare motor output pins:
/*  pinMode(DIO_HB_PWMLEFT, OUTPUT);
  pinMode(DIO_HB_EN1LEFT, OUTPUT);
  pinMode(DIO_HB_EN2LEFT, OUTPUT);
*/
}

void processCommand(unsigned char *command, int commandLength) {
  if(commandLength>5)
    return;
  // process command
  switch (command[0]) {
    case 0:
      // disable robot
      digitalWrite(DIO_HB_EN1LEFT, LOW);
      digitalWrite(DIO_HB_EN2LEFT, LOW);
      analogWrite(DIO_HB_PWMLEFT, 0);
//      println("Robot is disabled");
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    case 5:
      break;
    case 6:
      break;
    case 7:
      {
        // set single drive values
        int power = (((unsigned byte)command[1]) - 100) * 2.55;
        int r=RUDDER_INIT+(-((unsigned byte)command[2]-90));
        // set servo values
//        myservo.write(r);

        // create the deadzone to switch off motor
        if(power<2&&power>-2) {
          digitalWrite(DIO_HB_EN1LEFT, LOW);
          digitalWrite(DIO_HB_EN2LEFT, LOW);
        } else {
          //forward, pin controls
          if (power > 2) {
            digitalWrite(DIO_HB_EN1LEFT, HIGH);
            digitalWrite(DIO_HB_EN2LEFT, LOW);
          }
          //backward, pin controls
          if (power < -2) {
           digitalWrite(DIO_HB_EN1LEFT, LOW);
           digitalWrite(DIO_HB_EN2LEFT, HIGH);
          }
        }
    
        // write the power level
        if(power==255)
          digitalWrite(DIO_HB_PWMLEFT,HIGH);
        else
          analogWrite(DIO_HB_PWMLEFT, abs(power));
      }
      break;
  }
}



