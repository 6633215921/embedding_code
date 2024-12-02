#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID      "TMPL6eOSDGPC5"
#define BLYNK_TEMPLATE_NAME    "ESP32 Sensor 2"
#define BLYNK_AUTH_TOKEN       "bpDk7dflq_0u-V5HZuTOmtUfn28LjSIb"

#include <Arduino.h>
#include <BlynkSimpleEsp32.h>

#define LIGHTPIN 33
#define AUDIOPIN 34
#define CARBONPIN 32

BlynkTimer timer;
unsigned long samples = 0;
unsigned long totallight = 0;
unsigned long totalaudio = 0;
unsigned long totalcarbon = 0;
void serverTick();

void setup() {
  pinMode(LIGHTPIN, INPUT);
  pinMode(AUDIOPIN, INPUT);
  pinMode(CARBONPIN, INPUT);
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, "Rb-i", "frxsne5vz837k");
  timer.setInterval(1000L, serverTick);
}

void serverTick() {
  unsigned long l = 4095 - analogRead(LIGHTPIN);
  unsigned long a = analogRead(AUDIOPIN);
  unsigned long c = analogRead(CARBONPIN);

  Serial.printf("Light %d | Sound %d | Carbon %d", l, a, c);
  Serial.println();
  samples++;
  totallight += l;
  totalaudio += a;
  totalcarbon += c;
  if (samples >= 10) {
    float datalight = totallight / float(samples);
    float dataaudio = totalaudio / float(samples);
    float datacarbon = totalcarbon / float(samples);

    //MQ-7
    datacarbon = datacarbon/10;
    float slope = -0.7516072988;
    float A = 45.87510694;
    float Rseries = 1000;
    float V_Rseries = (datacarbon*5)/1023;
    float Rs = ((5-V_Rseries)/V_Rseries)*Rseries;
    float R0 = 400;
    float Y = Rs/R0;
    datacarbon = pow(10,(log10(Y/A)/slope))*3;
    //
    datalight = datalight/10;
    dataaudio = dataaudio/10;

    Blynk.virtualWrite(V0, datalight);
    Blynk.virtualWrite(V1, dataaudio);
    Blynk.virtualWrite(V2, datacarbon);

    Blynk.virtualWrite(V4, datalight, dataaudio, datacarbon, millis() / 1000);

    samples = 0;
    totallight = 0;
    totalaudio = 0;
    totalcarbon = 0;
  } 
  Blynk.virtualWrite(V3, round(millis() / 1000));
}

void loop() {
  Blynk.run();
  timer.run();
}