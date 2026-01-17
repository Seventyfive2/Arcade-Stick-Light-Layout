#include <Adafruit_NeoPixel.h>
#include <Servo.h>
#include <WiFiNINA.h>
#include <SPI.h>
#include <SD.h>
#include <arduino_secrets.h>

#define REQ_BUF_SZ 60
#define LED_PIN 2

#define LED_COUNT 8

#define SERVO_PIN 3

Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int ledBrightness = 50;

int ledColor[8][3] = 
{
  {255, 255, 255}, // 0
  {255, 255, 255}, // 1
  {255, 255, 255}, // 2
  {255, 255, 255}, // 3
  {255, 255, 255}, // 4
  {255, 255, 255}, // 5
  {255, 255, 255}, // 6
  {255, 255, 255}  // 7
};

bool gateToggle = false;

Servo myservo;

char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

File webFile;
String pagePath = "/Webpage/index.htm";

char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer

boolean LED_status = 0;

WiFiServer server(80);  // create a server at port 80

void setup()
{
    //Initialize serial and wait for port to open:

    //Serial.begin(9600);

    //while (!Serial) {;} 

    HardwareSetup();

    ServerSetup();

    //SaveLayout("Test.xml");
}

void HardwareSetup()
{
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(10)) 
    {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");

    pixels.setBrightness(ledBrightness);

    pixels.begin();
    
    myservo.attach(SERVO_PIN);

    LoadLayout("Layout.xml");
}

void ServerSetup()
{
    // check for index.htm file
    if (!SD.exists("/Webpage/index.htm")) 
    {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");

    Serial.println("Access Point Web Server");

    // check for the WiFi module:

    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");

        // don't continue

        while (true);
    }

    String fv = WiFi.firmwareVersion();

    if (fv < WIFI_FIRMWARE_LATEST_VERSION) 
    {
        Serial.println("Please upgrade the firmware");
    }

    // by default the local IP address of will be 192.168.4.1

    // you can override it with the following:

    // WiFi.config(IPAddress(10, 0, 0, 1));

    // print the network name (SSID);

    Serial.print("Creating access point named: ");

    Serial.println(ssid);

    // Create open network. Change this line if you want to create an WEP network:

    status = WiFi.beginAP(ssid, pass);

    if (status != WL_AP_LISTENING) 
    {

        Serial.println("Creating access point failed");
        Serial.println(status);

        //pinMode(2, OUTPUT);

        // don't continue

        while (true);
    }

    delay(10000);

    server.begin();           // start to listen for clients

    printWiFiStatus();
}

void loop()
{
    WiFiClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank)
                {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    if (StrContains(HTTP_req, "layout"))
                    {
                        // send rest of HTTP header
                        client.println("Content-Type: application/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send XML file containing input states
                        //XML_response(client);
                        webFile = SD.open("Layout.xml"); //Open Current Layout
                        if (webFile) 
                        {
                            while(webFile.available()) 
                            {
                                //Serial.println(webFile.readStringUntil('\n').c_str());
                                client.write(webFile.read()); // send Current Layout to client
                            }
                            webFile.close();
                        }
                    }
                    else if(StrContains(HTTP_req, "POST"))
                    {
                        char* inputs = strtok(HTTP_req,"&");

                        while(inputs != 0)
                        {
                            if(StrContains(inputs, "4Way"))
                            {
                                if(StrContains(inputs+5, "true"))
                                {
                                    ChangeGate(true);
                                }
                                else
                                {
                                    ChangeGate(false);
                                }

                                SaveLayout("Layout.xml");
                            }
                            if(StrContains(inputs, "BTN"))
                            {
                                char hexCode[7];
                                strncpy(hexCode, inputs+5, 6);
                                hexCode[6] = '\0';  
                                
                                ChangeColor((int)inputs[3]-'0',hexCode);
                                UpdateButtons();
                                SaveLayout("Layout.xml");
                            }
                            if(StrContains(inputs, "Save"))
                            {
                                char buf[21];

                                strcpy(buf, "/Presets/");
                                strcat(buf, inputs+5);
                                strcat(buf, ".xml");
                                
                                SaveLayout(buf);
                            }
                            if(StrContains(inputs, "Load"))
                            {
                                char buf[21];

                                strcpy(buf, "/Presets/");
                                strcat(buf, inputs+5);
                                //strcat(buf, ".xml");
                                
                                LoadLayout(buf);
                            }
                            if(StrContains(inputs, "Delete"))
                            {
                                char buf[21];

                                strcpy(buf, "/Presets/");
                                strcat(buf, inputs+5);
                                //strcat(buf, ".xml");
                                
                                DeleteLayout(buf);
                            }
                            // Find the next command in input string
                            inputs = strtok(0, "&");
                        }
                    }
                    else if (StrContains(HTTP_req, "presets"))
                    {
                        File root = SD.open("/Presets/");

                        client.println("Content-Type: application/xml");
                        client.println("Connection: keep-alive");
                        client.println();

                        client.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                        client.println("<Presets>");

                        while (true)
                        {
                            File entry =  root.openNextFile();

                            if (! entry) 
                            {
                                // no more files
                                break;
                            }

                            client.println("<layout>");
                            
                            client.println(entry.name());
                            
                            client.println("</layout>");
                            entry.close();
                        }

                        client.println("</Presets>");
                    }

                    else if (StrContains(HTTP_req, "CSS"))
                    {
                        client.println("Content-Type: text/css");
                        client.println();
                        // send web page
                        webFile = SD.open("/Webpage/CSS/Main.css");        // open web page file
                        if (webFile) 
                        {
                            while(webFile.available()) 
                            {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }

                    else if (StrContains(HTTP_req, "JS"))
                    {
                        client.println("Content-Type: application/x-javascript");
                        client.println();
                        // send web page
                        webFile = SD.open("/Webpage/JS/Arduino.js");        // open web page file
                        if (webFile) 
                        {
                            while(webFile.available()) 
                            {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }
                    else
                    {   // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        //client.println("Connection: keep-alive");
                        client.println();
                        // send web page
                        webFile = SD.open(pagePath);        // open web page file
                        if (webFile) 
                        {
                            while(webFile.available()) 
                            {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }
                    // display received HTTP request on serial port
                    //Serial.print(HTTP_req);
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
}

void printWiFiStatus() {

  // print the SSID of the network you're attached to:

  Serial.print("SSID: ");

  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:

  IPAddress ip = WiFi.localIP();

  Serial.print("IP Address: ");

  Serial.println(ip);

  // print where to go in a browser:

  Serial.print("To see this page in action, open a browser to http://");

  Serial.println(ip);

}

void ChangeGate(bool fourWay)
{
    if(fourWay)
    {
        //Serial.println("Setting Gate to 4-Way.");
        myservo.write(45);
    }
    else
    {
        //Serial.println("Setting Gate to 8-Way.");
        myservo.write(0);
    }
  
    gateToggle = fourWay;
}

// checks if received HTTP request is switching on/off LEDs
// also saves the state of the LEDs
void ChangeColor(int id, char* input)
{
    char buffer[0];

    //Set Red
    buffer[0] = input[0];
    buffer[1] = input[1];
    ledColor[id][0] = StrToHex(buffer);

    //Set Green
    buffer[0] = input[2];
    buffer[1] = input[3];
    ledColor[id][1] = StrToHex(buffer);

    //Set Blue
    buffer[0] = input[4];
    buffer[1] = input[5];
    ledColor[id][2] = StrToHex(buffer);
}

void UpdateButtons()
{
  pixels.clear();

  for(int i=0; i<LED_COUNT; i++)
  {
    pixels.setPixelColor(i, pixels.Color(ledColor[i][0], ledColor[i][1], ledColor[i][2]));

    Serial.print("Button #");
    Serial.print(i);

    Serial.print(": Red: ");
    Serial.print(ledColor[i][0]);
    Serial.print(" Green: ");
    Serial.print(ledColor[i][1]);
    Serial.print(" Blue: ");
    Serial.println(ledColor[i][2]);
    //Serial.println(pixels.getPixelColor(i));
  }

  pixels.show();
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

String RGBtoHexStr(int r, int g, int b)
{
    String hexString = "#";

    if(r < 16) { hexString += "0"; }
    hexString += String(r, HEX);

    if(g < 16) { hexString += "0"; }
    hexString += String(g, HEX);
    
    if(b < 16) { hexString += "0"; }
    hexString += String(b, HEX);

    return hexString;
}

int StrToHex(char str[])
{
  return (int) strtol(str, 0, 16);
}

void SaveLayout(char * fileName)
{
    File saveFile;

    if(SD.exists(fileName))
    {
        //Overwrite file
        saveFile = SD.open(fileName, O_RDWR); 
    }
    else
    {
        //New Preset
        saveFile = SD.open(fileName, FILE_WRITE);
    }
    

    if (saveFile) 
    {
        Serial.print("Writing to ");
        Serial.println(fileName);

        saveFile.seek(0);
        saveFile.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        saveFile.println("<inputs>");
        saveFile.println("<fourWay>");
        if(gateToggle)
        {
            saveFile.println("true");
        }
        else
        {
            saveFile.println("false");
        }
        saveFile.println("</fourWay>");
        for(int i=0; i<LED_COUNT; i++)
        {
            saveFile.println("<button>");
            saveFile.println(RGBtoHexStr(ledColor[i][0], ledColor[i][1], ledColor[i][2]));
            saveFile.println("</button>");
        }
        saveFile.println("</inputs>");

        saveFile.close();
    }
}

void LoadLayout(char * fileName)
{
    File presetFile = SD.open(fileName); //Open Layout

    if (presetFile) 
    {
        Serial.print("Loading ");
        Serial.println(fileName);

        int btnNum = 0;

        while(presetFile.available())
        {
            String line = presetFile.readStringUntil('\n');
            line.trim();

            if(line.indexOf("false") != -1 || line.indexOf("true") !=-1)
            {
                if(line == "true")
                {
                    ChangeGate(true);
                }
                else
                {
                    ChangeGate(false);
                }
            }
            if(line.indexOf("#") != -1)
            {
                char hexCode[7];
                strncpy(hexCode, line.c_str()+1,6);
                hexCode[6] = '\0'; 

                ChangeColor(btnNum,hexCode);
                btnNum++;
            }
        }

        UpdateButtons();

        SaveLayout("Layout.xml");

        presetFile.close();
    }
    else
    {
        Serial.println("ERROR - Can't find Layout.xml file!");
    }
}

void DeleteLayout(char * fileName)
{
    if(SD.exists(fileName))
    {
        SD.remove(fileName);
    }
}