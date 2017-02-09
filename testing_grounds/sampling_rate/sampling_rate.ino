void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

long tend,r;

void loop() {
  // put your main code here, to run repeatedly:
  r=0;
  tend=micros()+1000000;
  while(micros()<tend){
    analogRead(17);
    r++;
  }
  Serial.println(r);
}
