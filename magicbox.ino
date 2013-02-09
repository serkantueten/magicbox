#include "DHT.h"
#include "Ethernet.h"
#include "SPI.h"
#include "SmoothAnalogInput.h"
#include "looper.h"

#define DHT1PIN 2
#define DHT2PIN 3

/*
  Relais Pin und Status.
*/
int relay1Pin = 9; // Lüfter
boolean relay1Status = LOW;

int relay2Pin = 8; // Radiator
boolean relay2Status = LOW;

/*
  Lüfter.
*/
int fanPinL = 5; // an analog PWM pin
int fanPinR = 6;

int fanSpeedL = 255; // max. 255 = 100%
int fanSpeedR = 255; //

int fanSpeedMax = 210;
int fanSpeedMin = 175;

/*
  Temperatur und RLF, 2 x DHT22.
*/
float t1, h1, t2, h2;
float normalTemp = 17.5;

DHT DHT1( DHT1PIN , DHT22 ); // Sensor 1
DHT DHT2( DHT2PIN , DHT22 ); // Sensor 2

/*
  Topfsensoren.
*/
int topfSensor1Pin = A1;
int topfSensor1Wert = 0;
SmoothAnalogInput ai1; 

/*
  Wasser & Giessen.
*/
int minWater = 550;
int giesszeitsek = 10;
boolean heuteGegossen = false;
boolean taghell = false;

/*
  Lichtsensor.
*/
int photocellPin = A0; // the cell and 10K pulldown are connected to a0
//int photocellReading = 0; // the analog reading from the sensor divider
//SmoothAnalogInput ai0;

/*
  Ethernet.
*/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char serverName[] = "www.panotrails.ch";
byte ip[] = { 192, 168, 10, 77 };

// the router's gateway address:
byte gateway[] = { 192, 168, 10, 1 };

// the subnet:
byte subnet[] = { 255, 255, 255, 0 };

EthernetClient client;

/*
  Looper, wird anstelle delay() verwendet.
*/
looper myScheduler;

/*
  Serial.write wird aktiviert.
*/
boolean debug = true;


/*
  Setup.
*/
void setup() {
  
  Serial.begin(9600);
  
  /*
    SD Card deaktivieren.
  */
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
  
  delay(1000);
  
  //while (!Serial); // wait for serial port to connect. Needed for Leonardo only
  
  /**/
  // Ethernet Verbindung aufbauen.
  Ethernet.begin(mac, ip, gateway, subnet);
  
  // Ethernet etwas Zeit geben.
  delay(1000);
  
  // Analog Input smoothen.
  //ai0.attach(photocellPin);
  ai1.attach(topfSensor1Pin);
  
  // initialize the relay pin as an output:
  pinMode( relay1Pin , OUTPUT );
  pinMode( relay2Pin , OUTPUT );
  
  // Temp + RFL
  DHT1.begin();
  DHT2.begin();
  
  // Relais 1 - Ventilatoren einschalten
  //relayToggler();
  
  // Lüfter
  pinMode(fanPinL, OUTPUT);
  pinMode(fanPinR, OUTPUT);
  
  // Lüfter einschalten
  //analogWrite(fanPinL, fanSpeedL);
  //analogWrite(fanPinR, fanSpeedR);
  
  updateSensorValues();
  
  /*
    Tasks an Scheduler hinzufügen.
  */
  myScheduler.addTask(updateSensorValues, 5000); // 5 sek
  myScheduler.addTask(inetConnectURL, 60000); // 1 min
  myScheduler.addTask(checkTemp, 10000); // 10 sek
  //myScheduler.addTask(luefterLogik, 10000); // 10 sek
  //myScheduler.addTask(waterCheck, 900000); // 15 min
  //myScheduler.addTask(relayToggler, 300000); // 5 min
  
}


/*
  Endlos.
*/
void loop() {
  
  myScheduler.scheduler();
  
  /*
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  */
  
}


/*
  Wasserpumpe.
  Einmal am Tag, wenn Sonne scheint, wird grosszügig gegossen. Möglichst früh gleich nach Sonnenaufgang. Damit
  gegossen wird, muss die Erde schon sehr trocken sein. Sensorwert bei etwa 550 (y). Nachts wird nie gegossen.
  Max. Giesszeit muss eine Menge von ca. 1 L (x) entweichen lassen. Dadurch wird pro Tag nie mehr als 1 L (x) gegossen.
*/
void waterCheck(){
  
  Serial.print("Pruefe Feuchtigkeit im Topf...");
  
  // Wenn Erde trocken & Licht ein und kein Wasser im Topfboden
  if (topfSensor1Wert < minWater) {
    Serial.print(" beginne zu giessen... ");
    digitalWrite( relay2Pin , HIGH );
    delay(1000);
    
    // Giesse solange trocken
    /*
    while(ai1.raw() < minWater) {
      analogRead(topfSensor1Pin);
      Serial.print("+ ");
      delay(1000);
      topfSensor1Wert = analogRead(topfSensor1Pin);
      Serial.print(topfSensor1Wert);
      delay(1000);
    }
    */
  
  }
  else {
    digitalWrite( relay2Pin , LOW );
  }
  
  // turn relay off:
  // digitalWrite( relay2Pin , LOW );
  Serial.println(" ok!");
  
  Serial.println("");
  
}



/*
  Radiator Relais.
*/
void checkTemp(){
  
  Serial.print("Temperaturcheck... ");
  
  // Wenn Temperatur zu kalt
  if (t1 < normalTemp)
  {
    if(relay2Status == LOW)
    {
      Serial.println("Temperatur zu tief, Radiator-Relais wird aktiviert!");
      digitalWrite( relay2Pin , HIGH );
    }
    else
    {
      Serial.println("Temperatur immer noch tief, Radiator-Relais bleibt aktiv.");
    }
  }
  else if (t1 > normalTemp + (normalTemp / 100))
  {
    Serial.println("Temperatur OK, Radiator-Relais deaktivieren.");
    digitalWrite( relay2Pin , LOW );
  }
  else
  {
     Serial.println("Temperatur OK, Radiator-Relais deaktiv.");
     digitalWrite( relay2Pin , LOW );
  }
  
  Serial.println("");
  
}



/*
  Lüfterlogik ohne grosse Logik.
*/
void luefterLogik(){

  fanSpeedL = random(fanSpeedMin, fanSpeedMax);
  fanSpeedR = random(fanSpeedMin, fanSpeedMax);
  
  analogWrite(fanPinL, fanSpeedL);
  analogWrite(fanPinR, fanSpeedR);
  
}


void inetConnectURL(){

  // Daten an URL senden
  Serial.print("### Datenbank Log schreiben... ");
  
  char temp[10];
  String floatAsString;
  
  /*
  // Ethernet Verbindung aufbauen.
  */
  Ethernet.begin(mac, ip, gateway, subnet);
  
  // Ethernet etwas Zeit geben.
  delay(500);
  
  
  // if you get a connection, report back via serial:
  if (client.connect(serverName, 80)) {
    Serial.print("verbunden... ");
    
    String data;
    
    data = data + photocellReading + ',';
    
    // float to string
    dtostrf(t1,1,2,temp);
    floatAsString = String(temp);
    
    data = data + String(floatAsString) + ",";
    
    dtostrf(h1,1,2,temp);
    floatAsString = String(temp);
    
    data = data + String(floatAsString) + ",";
    
    dtostrf(t2,1,2,temp);
    floatAsString = String(temp);
    
    data = data + String(floatAsString) + ",";
    
    dtostrf(h2,1,2,temp);
    floatAsString = String(temp);
    
    data = data + String(floatAsString) + ",";
    
    data = data + topfSensor1Wert + ",";
    
    data = data + relay1Status + ",";
    
    data = data + fanSpeedL + ",";
    
    data = data + fanSpeedR;
    
    // Make a HTTP request:
    client.print("GET /magicbox/cfc_datalog.cfc?method=logDataToDb&sensorWerte=");
    client.print(data);
    client.println(" HTTP/1.1");
    
    client.println("From: serkan@tueten.ch");
    
    client.println("Host: www.panotrails.ch");
    
    client.println("Connection: close");
    client.println("User-Agent: HTTPTool/1.1");
    
    client.println();
    
    Serial.println("ok!");
    
  } else {
    // if you couldn't make a connection:
    Serial.println("Verbindung fehlgeschlagen  - Verbindung wird geschlossen.");
  }
  
  Serial.println("");
  
  delay(1);
  
  client.stop();
  
}


/*
  Schaltet 2 Relays für die 4 Lüfter.
*/
void relayToggler(){

  if ( relay1Status == LOW ) {
    // turn relay on:
    digitalWrite( relay1Pin , HIGH );
  }
  else {
    // turn relay off:
    digitalWrite( relay1Pin , LOW );
  }

}


/*
  Für Debugging.
*/
void serialPrint(){

  Serial.print( "Sensor 1 - RLF: "); 
    Serial.print(h1);
    Serial.print(" %\t");
    Serial.print("Temp: ");
    Serial.print(t1);
    Serial.println(" C");
    
    Serial.print( "Sensor 2 - RLF: " ); 
    Serial.print( h2 );
    Serial.print( " %\t" );
    Serial.print( "Temp: " );
    Serial.print( t2 );
    Serial.println( " C" );
    
    Serial.print( "Topf 1 Feuchtigkeit: " );
    Serial.println( topfSensor1Wert );
    
    Serial.print( "Helligkeit: " );
    Serial.println( photocellPin );
    
    Serial.print( "Relais 2 Radiator: " );
    Serial.println(relay1Status);
    
    Serial.print( "Relais 1 Luefter: " );
    Serial.println(relay1Status);
    
    Serial.print( "Luefter links: " );
    Serial.print(fanSpeedL);
    Serial.print( " - rechts: " );
    Serial.println(fanSpeedR);
    
    Serial.println("");

}


/*
  Aktualisiere Sensorwerte.
*/
void updateSensorValues(){

  Serial.print("Aktualisiere Sensorvariablen... ");
  
  h1 = DHT1.readHumidity();
  t1 = DHT1.readTemperature();
  
  h2 = DHT2.readHumidity();
  t2 = DHT2.readTemperature();
  
  photocellReading = A0.read();
  
  topfSensor1Wert = ai1.read();
  
  relay1Status = digitalRead(relay1Pin);
  relay2Status = digitalRead(relay2Pin);
  
  Serial.println("ok!");
  Serial.println("");
  
  if(debug){
    serialPrint();
  }

}
