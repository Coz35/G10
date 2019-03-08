/*
Ground node sofware for storing temperature data incrementally, and then sending this data to 
the drone node upon a BLE connection.
*/
//#if SOFTWARE_SERIAL_AVAILABLE
//  #include <SoftwareSerial.h>
//#endif
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h> // Used to establish serial communication on the I2C bus

#include "SparkFunTMP102.h" // Used to send and recieve specific information from our sensor
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h" //Custom config file included in all Adafruit examples
#include "Adafruit_BLEGatt.h"

/* SD Global file variable */
File temperatureFile;

/* TMP 102 Global Variables */
//*************************************************************************************
// Connections
// VCC = 3.3V
// GND = GND
// SDA = A4
// SCL = A5
const int ALERT_PIN = A3;

// Sensor address can be changed with an external jumper to:
// ADD0 - Address
//  VCC - 0x49
//  SDA - 0x4A
//  SCL - 0x4B
TMP102 sensor0(0x48); // Initialize sensor at I2C address 0x48
//*************************************************************************************

/* Adafruit BLE global variables */
//*************************************************************************************
//Create global BLE object -> software SPI
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLEGatt gatt(ble);

//Custom implemented transparent UART service information
int32_t serviceId;
int32_t txCharId;
int32_t rxCharId;

/* Helper function */
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
//*************************************************************************************


/* Setup Code -> Run Once */
void setup() {
  Serial.begin(115200); // Start serial communication at 115200 baud

  /*TMP 102 Setup*/
  //***************************
  pinMode(ALERT_PIN, INPUT);  // Declare alertPin as an input
  sensor0.begin();  // Join I2C bus

  // set the sensor in Comparator Mode (0) or Interrupt Mode (1).
  sensor0.setAlertMode(0); // Comparator Mode.

  // set the Conversion Rate (how quickly the sensor gets a new reading)
  //0-3: 0:0.25Hz, 1:1Hz, 2:4Hz, 3:8Hz
  sensor0.setConversionRate(1);

  //set Extended Mode.
  //0:12-bit Temperature(-55C to +128C) 1:13-bit Temperature(-55C to +150C)
  sensor0.setExtendedMode(0);
  //***************************

  /* SD Card Setup */
  //***************************
  Serial.print("Initializing SD card...");

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  //***************************

  /*Adafruit BLE Setup*/
  //***************************
  boolean success;

  /* Initialize the module */
  Serial.print(F("Initializing the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  /* Perform a factory reset to make sure everything is in a known state */
  Serial.println(F("Performing a factory reset: "));
  if (! ble.factoryReset() ) {
    error(F("Couldn't factory reset"));
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  /* Change the device name to make it easier to find */
  Serial.println(F("Setting device name to 'Ground Node': "));

  if (! ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Ground Node")) ) {
    error(F("Could not set device name?"));
  }

  /* Add the UART Service Definition */
  uint8_t serviceUUID[16] = { 0x49, 0x53, 0x53, 0x43, 0xFE, 0x7D, 0x4A, 0xE5, 0x8F, 0xA9, 0x9F, 0xAF, 0xD2, 0x05, 0xE4, 0x55 };

  /* Service ID should be 1 */
  Serial.println(F("Adding the transparent UART service definition (UUID = 0x----): "));
  serviceId = gatt.addService(serviceUUID);
  if (! serviceId)
  {
    error(F("Could not add UART service"));
  }

  /* Add custom defined UART TX characteristic */
  uint8_t txCharUUID[16] = { 0x49, 0x53, 0x53, 0x43, 0x1E, 0x4D, 0x4B, 0xD9, 0xBA, 0x61, 0x23, 0xC6, 0x47, 0x24, 0x96, 0x16 };

  /* Characteristic ID should be 1 */
  Serial.println(F("Adding transparent UART TX characteristic (UUID = 0x----): "));
  txCharId = gatt.addCharacteristic(txCharUUID, GATT_CHARS_PROPERTIES_INDICATE | GATT_CHARS_PROPERTIES_WRITE_WO_RESP | GATT_CHARS_PROPERTIES_WRITE | GATT_CHARS_PROPERTIES_NOTIFY, 1, 20, BLE_DATATYPE_AUTO);
  if (! txCharId)
  {
    error(F("Could not add UART TX characteristic"));
  }

  /* Add custom defined UART TX characteristic */
  uint8_t rxCharUUID[16] = { 0x49, 0x53, 0x53, 0x43, 0x88, 0x41, 0x43, 0xF4, 0xA8, 0xD4, 0xEC, 0xBE, 0x34, 0x72, 0x9B, 0xB3 };

  /* Characteristic ID should be 2 */
  Serial.println(F("Adding transparent UART RX characteristic (UUID = 0x----): "));
  rxCharId = gatt.addCharacteristic(rxCharUUID, GATT_CHARS_PROPERTIES_WRITE_WO_RESP | GATT_CHARS_PROPERTIES_WRITE, 1, 20, BLE_DATATYPE_AUTO);
  if (! rxCharId)
  {
    error(F("Could not add UART RX characteristic"));
  }

  /* Reset the device for the new service setting changes to take effect */
  Serial.print(F("Performing a SW reset (service changes require a reset): "));
  ble.reset();

  // Change Mode LED Activity
  Serial.println(F("Change LED activity to 4"));
  ble.sendCommandCheckOK("AT+HWModeLED=4");

  //Increase power
  Serial.println(F("Increase power level to 4"));
  ble.sendCommandCheckOK("AT+BLEPOWERLEVEL=4");

  ble.verbose(false);

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println();
  //***************************
}

// put your main code here, to run repeatedly:
void loop() {

  if (ble.isConnected())
  {
    Serial.println("A connection has been formed!");

    while (ble.isConnected())
    {
      long start_time = millis();

      /* SD card had notified on property */
      if (gatt.getCharInt8(rxCharId) == 0xFF)
      {
        gatt.setChar(rxCharId, 0x00); //Return the RxChar ID value to zero

        temperatureFile = SD.open("tmp.txt"); //Open the temperature file for reading

        Serial.print(temperatureFile.size()); //Some information for timing purposes
        Serial.println(" bytes");

        char buf[20];

        if (temperatureFile) //File opened properly
        {
          while (temperatureFile.available() && ble.isConnected())
          {                                              
            /* Data request from the drone node */
            if (gatt.getCharInt8(rxCharId) == 0xEE)
            {                           
              gatt.setChar(rxCharId, 0x00);

              char temp[80];

              /* Read stored temperature data from SD card */
              for (int i = 0; i < 80; i++)
              {
                temp[i] = (char)temperatureFile.read();
              }

              /* Send temperature data to drone node in 80 byte chunk */
              for (int count = 0; count < 80; count = count + 20)
              {
                int j = 0;

                for (int i = count; i < count + 20; i++)
                {
                  buf[j] = temp[i];
                }

                /* Send 20 byte chunk */
                Serial.print(buf);
                gatt.setChar(txCharId, buf); 
                
                Serial.println("80 bytes sent");
              }
            }
          }

          /* Transmission over, report elapsed time */
          Serial.print("Time (s): ");
          Serial.println((float)(millis() - start_time) / 1000);

          delay(2000);

          /* Send stop codes */
          for (int i = 0; i < 20; i++)
          {
            buf[i] = (char)0xFF;
          }
          gatt.setChar(txCharId, buf);
        }

        temperatureFile.close();
      }
    }
  }
  else
  {
    /* Gather temperature readings */
    //***************************
    float temperature;

    // Turn sensor on to start temperature measurement.
    sensor0.wakeup();

    // read temperature data
    temperature = sensor0.readTempC();

    // Place sensor in sleep mode to save power.
    sensor0.sleep();
    //***************************

    /* Write the temperature to file */
    //***************************
    temperatureFile = SD.open("tmp.txt", FILE_WRITE);

    if (temperatureFile)
    {
      String tempString;

      temperatureFile.print("1 "); //Ground node ID of 1
      temperatureFile.print(millis() / 1000);
      temperatureFile.print(" ");
      temperatureFile.print(temperature);
      temperatureFile.print(",");
    }
    else
    {
      Serial.println("Error opening temperature file");
    }

    temperatureFile.close(); //File must be closed and reopened, otherwise this does not work
    //***************************
  }
  
  delay(1000);  // Wait 1000ms
}
