#include "BLEDevice.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>

#define ADXL343_SCK 13
#define ADXL343_MISO 12
#define ADXL343_MOSI 11
#define ADXL343_CS 10
// Define the UUIDs of the service and characteristic you want to interact with
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
bool ledONR;
bool ledONL;

// ACCELEROMETER SETUP
/* Assign a unique ID to this sensor at the same time */
/* Uncomment following line for default Wire bus      */
Adafruit_ADXL343 accel = Adafruit_ADXL343(12345);

/* NeoTrellis M4, etc.                    */
/* Uncomment following line for Wire1 bus */
//Adafruit_ADXL343 accel = Adafruit_ADXL343(12345, &Wire1);

/* Uncomment for software SPI */
//Adafruit_ADXL343 accel = Adafruit_ADXL343(ADXL343_SCK, ADXL343_MISO, ADXL343_MOSI, ADXL343_CS, 12345);

/* Uncomment for hardware SPI */
//Adafruit_ADXL343 accel = Adafruit_ADXL343(ADXL343_CS, &SPI, 12345);

// NEOPIXEL SETUP
int LEDPINR = 33;
int LEDPINL = 15;
int NUMPIXELS = 3; // number of Neopixels
Adafruit_NeoPixel pixelsR = Adafruit_NeoPixel(NUMPIXELS, LEDPINR, NEO_RGBW + NEO_KHZ800);
Adafruit_NeoPixel pixelsL = Adafruit_NeoPixel(NUMPIXELS, LEDPINL, NEO_RGBW + NEO_KHZ800);

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if (length == 1 && pData[0] == 1) {
        Serial.println("Turning on right");
        ledONR = true;
    }
    if (length == 1 && pData[0] == 0) {
      Serial.println("Turning off right");
      ledONR = false;
    }
    if (pData[0] == 2) {
        Serial.println("Turning on left");
        ledONL = true;
    }
    if (length == 1 && pData[0] == 0) {
      Serial.println("Turning off left");
      ledONL = false;
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // Connect to the server
    Serial.println(" - Connected to server");
    pClient->setMTU(517); // Request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if (pRemoteCharacteristic->canRead()) {
  //std::string value = pRemoteCharacteristic->readValue(); THIS DOESN'T WORK FOR SOME REASON
  //String arduinoString = String(value.c_str());
  Serial.print("The characteristic value was: ");
 // Serial.println(arduinoString);
}


    // Register for notifications
    if(pRemoteCharacteristic->canNotify()) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
      Serial.println("Registered for notifications");
    }

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Check if the device advertises the desired service
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback for when a new device is detected
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  // initalizes neopixels and sets to off
  pixelsR.begin();
  pixelsR.show();
  pixelsL.begin();
  pixelsL.show();
  // set up speaker
  ledcAttach(12,440,12);
   /* Initialise accelerometer */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL343 ... check your connections */
    Serial.println("Ooops, no ADXL343 detected ... Check your wiring!");
    while(1);
  }
  /* Set the range to whatever is appropriate for your project */
  //accel.setRange(ADXL343_RANGE_16_G);
  // accel.setRange(ADXL343_RANGE_8_G);
  // accel.setRange(ADXL343_RANGE_4_G);
   accel.setRange(ADXL343_RANGE_2_G);
}

void loop() {
  // If the flag "doConnect" is true, connect to the server
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to the server.");
    }
    doConnect = false;
  }

  // Your main code here
    if (ledONR) {
      ledcWrite(12,1000);
        pixelsR.clear();
        pixelsR.setPixelColor(0,pixelsR.Color(0,255,0,0));
        pixelsR.setPixelColor(1,pixelsR.Color(0,255,0,0)); 
        pixelsR.setPixelColor(2,pixelsR.Color(0,255,0,0));
        pixelsR.show();
        delay(550);
        pixelsR.clear();
        pixelsR.show();
        delay(550);
    } else {
      ledcWrite(12,0);
      pixelsR.clear();
      pixelsR.show();
    }

    if (ledONL) {
      ledcWrite(12,1000);
        pixelsL.clear();
        pixelsL.setPixelColor(0,pixelsL.Color(0,255,0,0));
        pixelsL.setPixelColor(1,pixelsL.Color(0,255,0,0)); 
        pixelsL.setPixelColor(2,pixelsL.Color(0,255,0,0));
        pixelsL.show();
        delay(550);
        pixelsL.clear();
        pixelsL.show();
        delay(550);
    } else {
      ledcWrite(12,0);
      pixelsL.clear();
      pixelsL.show();
    }

  // HOW ACCELEROMETER IS READ, NOT USED YET

  //   /* Get a new sensor event */
  // sensors_event_t event;
  // accel.getEvent(&event);

  // /* Display the results (acceleration is measured in m/s^2) */
  // Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  // Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  // Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
}
