/**
*  @file - G191210088.ino
*  @description - Ayakkabıya yerleştirilen ağırlık sensörleri ile ayağın 
* -basma noktalarını internet ile ileten program kodu. 
*  @course - Birinci Öğretim A grubu
*  @date - 12.2022
*  @author - Bahadır Beşir Kestane - bahadir.kestane@ogr.sakarya.edu.tr
*/

#include "config.h" 
#include <HX711_ADC.h> // Ağırlık sensöründen gelen analog sinyali dijital sinyale dönüştüren dönüştürücü balık dosyası.
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

//pinler:

const int HX711_dout_1 = 14; //mcu > HX711 no 1 dout pin
const int HX711_sck_1 = 12; //mcu > HX711 no 1 sck pin
const int HX711_dout_2 = 13; //mcu > HX711 no 2 dout pin
const int HX711_sck_2 = 15; //mcu > HX711 no 2 sck pin



//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell_1(HX711_dout_1, HX711_sck_1); //HX711 1
HX711_ADC LoadCell_2(HX711_dout_2, HX711_sck_2); //HX711 2

AdafruitIO_Feed *text1 = io.feed("sensor1");
AdafruitIO_Feed *text2 = io.feed("sensor2");
AdafruitIO_Feed *text3 = io.feed("Text");

const int calVal_eepromAdress_1 = 0; // eeprom adress for calibration value load cell 1 (4 bytes)
const int calVal_eepromAdress_2 = 4; // eeprom adress for calibration value load cell 2 (4 bytes)
unsigned long t = 0;

void setup() {
  Serial.begin(115200); delay(10);
  Serial.println();
  Serial.println("Starting...");

  while(! Serial);

  Serial.print("Connecting to Adafruit IO");
  // connect to io.adafruit.com  
  io.connect();

  
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println(F("Kurulum Hazır"));

  float calibrationValue_1; // load cell 1 için kalibrasyon değeri.
  float calibrationValue_2; // load cell 2 için kalibrasyon değeri.

  calibrationValue_1 = 696.0; 
  calibrationValue_2 = 733.0; 
#if defined(ESP8266) || defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266 and want to fetch the value from eeprom
#endif
  //EEPROM.get(calVal_eepromAdress_1, calibrationValue_1); // uncomment this if you want to fetch the value from eeprom
  //EEPROM.get(calVal_eepromAdress_2, calibrationValue_2); // uncomment this if you want to fetch the value from eeprom

  LoadCell_1.begin();
  LoadCell_2.begin();
  //LoadCell_1.setReverseOutput();
  //LoadCell_2.setReverseOutput();
  unsigned long stabilizingtime = 2000; // Daranın daha doğru alınması için gereken bekleme süresi.
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  byte loadcell_1_rdy = 0;
  byte loadcell_2_rdy = 0;
  while ((loadcell_1_rdy + loadcell_2_rdy) < 2) { // İki ağırlık sensötü içinde başlatma işlemi.
    if (!loadcell_1_rdy) loadcell_1_rdy = LoadCell_1.startMultiple(stabilizingtime, _tare);
    if (!loadcell_2_rdy) loadcell_2_rdy = LoadCell_2.startMultiple(stabilizingtime, _tare);
  }
  
  LoadCell_1.setCalFactor(calibrationValue_1); // user set calibration value (float)
  LoadCell_2.setCalFactor(calibrationValue_2); // user set calibration value (float)
  Serial.println("Startup is complete");
}

void loop() {

  io.run();

  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell_1.update()) newDataReady = true;
  LoadCell_2.update();

  //get smoothed value from data set
  if ((newDataReady)) {
    if (millis() > t + serialPrintInterval) {
      float a = LoadCell_1.getData();
      float b = LoadCell_2.getData();
      Serial.print("Load_cell 1 output val: ");
      Serial.print(a);
      Serial.print("    Load_cell 2 output val: ");
      Serial.println(b);

      text1->save(a);
      text2->save(b);

      if(a<b)
      {
        text3->save("Ayağınızın sağ tarafına daha fazla basıyorsunuz.");                
      }
      else if(a>b)
      {
        text3->save("Ayağınızın sol tarafına daha fazla basıyorsunuz.") ;       
      }
      else
      {
        text3->save("Her iki tarafada aynı oranda basıyorsunuz.");
      }

      delay(10000);

      newDataReady = 0;
      t = millis();
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') {
      LoadCell_1.tareNoDelay();
      LoadCell_2.tareNoDelay();
    }
  }

  //check if last tare operation is complete
  if (LoadCell_1.getTareStatus() == true) {
    Serial.println("Tare load cell 1 complete");
  }
  if (LoadCell_2.getTareStatus() == true) {
    Serial.println("Tare load cell 2 complete");
  }

}