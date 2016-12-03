/*
 * test
 */ 
 
 int i = 1;
 
 void setup()
 {
   Serial.begin(9600);
   while (!Serial) {
   }
 }
 
 void loop()
 {
   Serial.println(i);
   delay(1000);
   i++;
 }
