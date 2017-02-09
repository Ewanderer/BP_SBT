#define data_count 10000

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(A3,INPUT);
  analogReadResolution(13);
}

uint16_t data[data_count];
uint16_t minv,maxv;
float medv;
void loop() {
  minv=10000;
  maxv=0;
  medv=0;
  for(int i=0;i<data_count;i++){
    data[i]=analogRead(A3);
    if(data[i]>maxv)
      maxv=data[i];
      if(data[i]<minv)
        minv=data[i];
  }
  Serial.println("Werte:");
  for(int i=0;i<data_count;i++){
    Serial.print(data[i]);
    medv+=data[i];
    Serial.print(",");
  }
  Serial.println("");
   medv=medv/data_count;
   Serial.println(maxv);
   Serial.println(minv);
   Serial.println(medv);
   Serial.println(maxv-minv);
   Serial.println(".");
 
}
