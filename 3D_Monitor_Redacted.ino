/*  Connects to the Flashforge Finder 3D printer and requests printing status information
     All printer codes taken from Wireshark traces during 3d print setup and progress
*/
#define BLYNK_PRINT Serial
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// UPDATE THESE SETTINGS
char ssid[] = "YOUR WIFI SSID";  // SSID of your home WiFi
char pass[] = "YOUR WIFI PASSWORD";            // password of your home WiFi
char auth[] = "YOUR BLYNK PROJECT AUTH TOKEN"; // taken from your Blynk Project
IPAddress server(192, 168, 0, 144);    // the fixed IP address that you set on the 3d Printer

// GENERAL VARIABLES
float pct = 0.0; // Used to calculate % printed
float printed = 0.0; // Used to calculate % printed
float lefttoprint = 0.0; // Used to calculate % printed
String answer = ""; // Response from Printer
String ServerCon = ""; // Printer connection status
String JobState = ""; // State of print job
int slashpos = 0; // used to find bytes printed value
int EmailSent = 0; // flag to indicate when an email has been sent so that we dont send multiple emails

// Variables for Elapsed Time
int hrs = 0;
int mins = 0;
int secs = 0;
byte hour = 0;
byte minute = 0;
byte second = 0;

WiFiClient client;

BlynkTimer timer; // Announcing the timer

void setup() {
  Serial.begin(115200);               // only for debug
  WiFi.begin(ssid, pass);             // connects to the WiFi router
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Blynk.begin(auth, ssid, pass); // start BLYNK server
  timer.setInterval(200L, SendBLYNK); //timer will run every 200 msec
  delay(500);
  // Reset Elapsed time in Blynk project
  Blynk.virtualWrite(V5, "Elapsed : " + String(String(hour) + " h " + String(minute) + " m " + String(second) + "s"));
  delay(100);
  Connecttoserver();
}

void loop () {
  Blynk.run();        // run Blynk
  timer.run();        // run Blynk timer
}

void Connecttoserver()
{
  if (client.connect(server, 8899)) // if we are connected to 3D printer
  {
    ServerCon = "FINDER CONNECTED";
  }
  else
  {
    ServerCon = "FINDER NOT CONNECTED";
    Blynk.virtualWrite(V1, String("0.0"));
    Blynk.virtualWrite(V2, "IDLE");
    Blynk.virtualWrite(V5, "0h 0m 0s");
    Blynk.virtualWrite(V4, "Status: READY");
    Blynk.virtualWrite(V3, "Nozzle °C :");

    //Connect to 3d printer
    client.connect(server, 8899);
    delay(50);
    //Wake up server
    SendToPrinter();
  }
}

void SendBLYNK()
{
  // elapsed time
  long now = millis();
  hour = (now / 3600000);
  now -= (hour * 3600000);
  minute = (now / 60000);
  now -= (minute * 60000);
  second = (now / 1000);
  now -= (second * 1000);

// check server connection
  if (client.connect(server, 8899))
  {
    ServerCon = "FINDER CONNECTED";
    // send commands to printer to wake up connection - taken from wireshark dump
    SendToPrinter();
    if (answer.indexOf("printing byte ") > 0) // only interested in the percentage complete
    {
      slashpos = answer.indexOf("/"); // find where the slash symbol is in the returned data for Bytes printed / Remaining

      // find start of printed value
      int i = slashpos - 1;
      while (i > slashpos - 10)
      {
        if (answer.substring(i, i + 1) == " ")
        {
          printed = answer.substring(i, slashpos).toInt();
          break;
        }
        i--;
      }

      // calculate and print the percentage completed.
      lefttoprint = answer.substring(slashpos + 1, (answer.length())).toInt();
      pct = (printed / lefttoprint) * 100;

      if (pct > 100.00) // have seen percentage rise to 100.01 so making sure it stops at 100%
      {
        pct = 100.00;
      }

      if (pct >= 100.00)
      {
        if (EmailSent == 0) // only send the email once
        {
          // send email to the address registered to your BLYNK axccount
          Blynk.email("Subject", "3D print COMPLETED - Elapsed Time : " + String(String(hour) + " h " + String(minute) + " m " + String(second) + "s"));
          EmailSent = 1;
        }
        JobState = "COMPLETED";
      }
      if ((pct > 0.00) && (pct < 100.00))
      {
        EmailSent = 0;
        JobState = "PRINTING";
      }
      if (pct == 0.00)
      {
        JobState = "IDLE";
      }
    }
    client.println("\x7e\x4d\x31\x31\x39\x0d\x0a");  // sends M119 message to the server for Nozzle status
    answer = client.readStringUntil('ok');   // receives the answer from the sever

    Blynk.virtualWrite(V4, "Status: " + answer.substring(answer.indexOf("MoveMode: ") + 10, answer.length()));

    client.println("\x7e\x4d\x31\x30\x35\x0d\x0a");  // sends M105 message to the server for Nozzle temp
    answer = client.readStringUntil('ok');   // receives the answer from the sever
    Blynk.virtualWrite(V3, "Nozzle Temp: " + answer.substring(answer.indexOf("T0:") + 3, answer.indexOf("T0:") + 6) + "°C");

    client.flush();
  }
  else
  {
    ServerCon = "FINDER NOT CONNECTED";
    Connecttoserver();
  }
  // Update Blynk project
  Blynk.virtualWrite(V0, ServerCon);
  Blynk.virtualWrite(V1, String(pct));
  Blynk.virtualWrite(V2, JobState);
  if (JobState != "COMPLETED")
  {
    Blynk.virtualWrite(V5, "Elapsed : " + String(String(hour) + " h " + String(minute) + " m " + String(second) + "s"));
  }
}

void SendToPrinter()
{
  // The following was taken from a Wireshark trace of a live 3D Print
  client.println("\x7e\x4d\x36\x30\x31\x20\x53\x31\x0d\x0a");  // sends M601 to the server
  answer = client.readStringUntil('ok');   // receives the answer from the sever

  client.println("\x7e\x4d\x31\x31\x35\x0d\x0a");  // sends M115 message to the server
  answer = client.readStringUntil('ok');   // receives the answer from the sever

  client.println("\x7e\x4d\x36\x35\x30\x0d\x0a");  // sends the message to the server
  answer = client.readStringUntil('ok');   // receives M650 answer from the sever

  client.println("\x7e\x4d\x31\x31\x35\x0d\x0a");  // sends M115 message to the server
  answer = client.readStringUntil('ok');   // receives the answer from the sever

  client.println("\x7e\x4d\x31\x31\x34\x0d\x0a");  // sends M114 message to the server
  answer = client.readStringUntil('ok');   // receives the answer from the sever

  client.println("\x7e\x4d\x32\x37\x0d\x0a");  // sends M27 message to the server
  answer = client.readStringUntil('ok');   // receives the answer from the sever
}
