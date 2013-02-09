/*
  Hole Bibliothek.
*/
#include "DHT.h"

/*
  Temperatur- und Feuchtigkeitssensor.
*/
int DHT1Pin = 2; // the cell and 10K pulldown are connected to 2
float temp1, hum1;

/*
  Zuweisen.
*/
DHT TempHum1( DHT1Pin , DHT22 ); // Sensor 1


/*
  Setup.
*/
void setup() {
  
  updateTempHum();
  
}

/*
  Endlos.
*/
void loop() {
 
  updateTempHum();
  
  Serial.print( "Sensor 1 Temperatur: " );
  Serial.print( temp1 );
  Serial.print( " C - Feuchtigkeit: " );
  Serial.print( hum1 );
  Serial.println( " %" );
  
  delay(2500);
  
}

/*
  Update Sensorwerte.
*/
void updateTempHum() {
 
  temp1 = TempHum1.readTemperature();
  hum1 = TempHum1.readHumidity();
  
}
