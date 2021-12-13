#include <SPI.h>
#include <BLEPeripheral.h>

#include "bigd.h"

#define RSA_SIZE_BYTES 64
#define CHAR_SIZE 16

//custom boards may override default pin definitions with BLEPeripheral(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheral blePeripheral = BLEPeripheral();

BLEService ledService = BLEService("4fafc2011fb5459e8fccc5c9c331914b");

BLECharacteristic txChar0 = BLECharacteristic("beb5483e36e14688b7f5ea07361b26a8", BLERead | BLENotify, CHAR_SIZE);
BLECharacteristic txChar1 = BLECharacteristic("beb5483e36e14688b7f5ea07361b26a9", BLERead | BLENotify, CHAR_SIZE);
BLECharacteristic txChar2 = BLECharacteristic("beb5483e36e14688b7f5ea07361b26aa", BLERead | BLENotify, CHAR_SIZE);
BLECharacteristic txChar3 = BLECharacteristic("beb5483e36e14688b7f5ea07361b26ab", BLERead | BLENotify, CHAR_SIZE);

BLECharacteristic rxChar0 = BLECharacteristic("6389fb4fd7a54a04a4166dfce3f0f14d", BLEWrite, CHAR_SIZE);
BLECharacteristic rxChar1 = BLECharacteristic("6389fb4fd7a54a04a4166dfce3f0f14e", BLEWrite, CHAR_SIZE);
BLECharacteristic rxChar2 = BLECharacteristic("6389fb4fd7a54a04a4166dfce3f0f14f", BLEWrite, CHAR_SIZE);
BLECharacteristic rxChar3 = BLECharacteristic("6389fb4fd7a54a04a4166dfce3f0f150", BLEWrite, CHAR_SIZE);

BIGD MODULUS, PRIVATE_EXPONENT, x, y;

uint8_t inbuf[RSA_SIZE_BYTES];
uint8_t outbuf[RSA_SIZE_BYTES];

volatile uint8_t recvChar;

void rx0Written(BLECentral& central, BLECharacteristic& characteristic) {
  recvChar |= 1;
}

void rx1Written(BLECentral& central, BLECharacteristic& characteristic) {
  recvChar |= 2;
}

void rx2Written(BLECentral& central, BLECharacteristic& characteristic) {
  recvChar |= 4;
}

void rx3Written(BLECentral& central, BLECharacteristic& characteristic) {
  recvChar |= 8;
}

void blePeripheralDisconnectHandler(BLECentral& c) {
  //blePeripheral.begin();
  //sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
  sd_nvic_SystemReset();
}

void setup() {
  bdNewVars(&MODULUS, &PRIVATE_EXPONENT, &x, &y, NULL);

  //KEY 1
  //bdConvFromHex(MODULUS, "00c38dec5ff37fbee973cc03e73d13757c537ad43a3ea41ced20193df66c337a4e3926d6d6e1af6e0738c2008fcde0a71ad9d22d394f8184c11116f68e719d96bd");
  //bdConvFromHex(PRIVATE_EXPONENT, "51bcc39d438914c23d8d7be02e8e30a03bc06e6ebdfa18c120968c68b0c73f8a319a2e3ec11b892e488361dc45e4dfd631bbb087e6a08026ea2c1003035270c1");

  //KEY 2
  //bdConvFromHex(MODULUS, "00bb41cb594985b095f2506037019d1dc97c8033b8fb8f9b1629a5adfbeecbb0f406cf5cf140ec3ed53f81c15df584ad22a22196131e0c7f6c6766dd804e9b2681");
  //bdConvFromHex(PRIVATE_EXPONENT, "2e1a21bd66cb8251386a2f75fb70ba2fabf64845a7b190662174c7e3f9c3ae59ca1117e985e4d2f34992ee9cfb3ea93e59f5d217682447edeeed80772c6fd61d");

  //KEY 3
  bdConvFromHex(MODULUS, "00ba693830d0b36a7e67ab4322991aa6b7b7599d7f5ed2900907fc6fdcef15de6d90d7559c7a60478dfaf3883a400849153ff29a0af2c8852221f9f122283ca059");
  bdConvFromHex(PRIVATE_EXPONENT, "392620eff64bcb0e4dc217a6f7c14ff36ae6b27e6617afc8d505f3558c86ebd5acff579c784559f483770a644ace7ef735d4464f2342d4125f23522aad145771");


  blePeripheral.setLocalName("SKFob");
  blePeripheral.setAdvertisedServiceUuid(ledService.uuid());
  blePeripheral.setAdvertisingInterval(0x600); //(1000);

  blePeripheral.addAttribute(ledService);
  blePeripheral.addAttribute(txChar0);
  blePeripheral.addAttribute(txChar1);
  blePeripheral.addAttribute(txChar2);
  blePeripheral.addAttribute(txChar3);
  blePeripheral.addAttribute(rxChar0);
  blePeripheral.addAttribute(rxChar1);
  blePeripheral.addAttribute(rxChar2);
  blePeripheral.addAttribute(rxChar3);

  //blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  rxChar0.setEventHandler(BLEWritten, rx0Written);
  rxChar1.setEventHandler(BLEWritten, rx1Written);
  rxChar2.setEventHandler(BLEWritten, rx2Written);
  rxChar3.setEventHandler(BLEWritten, rx3Written);

  blePeripheral.begin();

  sd_power_mode_set(NRF_POWER_MODE_LOWPWR);

  recvChar = 0;
}

const uint8_t ZEROS[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

void loop() {
  // poll peripheral
  sd_app_evt_wait();
  blePeripheral.poll();

  if (recvChar == 15) {
    if(rxChar0.valueLength() != 16 || rxChar1.valueLength() != 16 || rxChar2.valueLength() != 16 || rxChar3.valueLength() != 16) {
      txChar0.setValue(ZEROS, 16);
      txChar0.setValue(ZEROS, 16);
      txChar0.setValue(ZEROS, 16);
      txChar0.setValue(ZEROS, 16);
    } else {
      memcpy(inbuf, rxChar0.value(), rxChar0.valueLength());
      memcpy(inbuf + 16, rxChar1.value(), rxChar1.valueLength());
      memcpy(inbuf + 32, rxChar2.value(), rxChar2.valueLength());
      memcpy(inbuf + 48, rxChar3.value(), rxChar3.valueLength());

      bdConvFromOctets(x, inbuf, RSA_SIZE_BYTES);
      bdModExp(y, x, PRIVATE_EXPONENT, MODULUS);
      bdConvToOctets(y, outbuf, RSA_SIZE_BYTES);

      txChar0.setValue(outbuf, CHAR_SIZE);
      blePeripheral.poll();
      delay(10);
      txChar1.setValue(outbuf + 16, CHAR_SIZE);
      blePeripheral.poll();
      delay(10);
      txChar2.setValue(outbuf + 32, CHAR_SIZE);
      blePeripheral.poll();
      delay(10);
      txChar3.setValue(outbuf + 48, CHAR_SIZE);
      blePeripheral.poll();
      delay(10);
    }

    recvChar = 0;
  }
}