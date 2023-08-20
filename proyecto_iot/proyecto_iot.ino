int i=0;

void setup(void)
{
  Serial.begin(9600);
}

void loop() {
  i=i+2;
  i=i-1;
  Serial.println(i);
  delay(1000);
}