#define l 30000

void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);
}
int val,b,e;
uint8_t values[l];

void loop() {
  b=micros();
  for(int i=0;i<l;i++)
    values[i]=analogRead(17);
  e=micros();
  for(int i=0;i<l;i++)
    Serial.write(values[i]);
   Serial.write("\n");
  Serial.println(e-b);
  delay(5000);
}
