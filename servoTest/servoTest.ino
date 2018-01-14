#include <Servo.h>

Servo myServ;

void setup() {
  // put your setup code here, to run once:
  myServ.attach(9);
  myServ.write(90);
}

void loop() {
  // put your main code here, to run repeatedly:
  myServ.write(45);
  delay(150);
  myServ.write(90);
  delay(1000);
  myServ.write(135);
  delay(150);
  myServ.write(90);
  delay(1000);
}
