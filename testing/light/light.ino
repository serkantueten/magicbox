/*
  Lichtsensor.
*/
int photocellPin = A0; // the cell and 10K pulldown are connected to a0
int photocellReading = 0; // the analog reading from the sensor divider


/*
  Setup.
*/
void setup() {
  
}

/*
  Endlos.
*/
void loop() {
 
  updateLight();
  
  Serial.print( "photocellReading: " );
  Serial.println( photocellReading );
  
  delay(1000);
  
}

/*
  Update Sensorwerte.
*/
void updateLight() {
 
  photocellReading = analogRead(photocellPin);
  
}
