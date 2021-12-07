#include "FobAuth.h"
#include "esp_log.h"

FobAuth::FobAuth(const char* pubkeymod, const char* pubkeyexp) {
  bdNewVars(&MODULUS, &PUBLIC_EXPONENT, &x, &y, &yp, NULL);
  bdConvFromHex(MODULUS, pubkeymod);
  bdConvFromHex(PUBLIC_EXPONENT, pubkeyexp);
}

bool FobAuth::doAuth(BLE& ble) {
    //ESP_LOGE("FOBAUTH", "Initiating fob connect");
    if (!ble.connectToFob()) {
      ESP_LOGE("FOBAUTH", "Fob connect failed");
      return false;
    }
    //ESP_LOGE("FOBAUTH", "Fob connected, creating challenge string");

    uint8_t buf[RSA_SIZE_BYTES]; //challenge string for fob
    bdRandomNumber(yp, MODULUS);
    bdConvToOctets(yp, buf, RSA_SIZE_BYTES);

    //ESP_LOGE("FOBAUTH", "Writing to fob");
    ble.fobWrite(buf);
    //ESP_LOGE("FOBAUTH", "Fob write done, receiving");
    ble.fobRecv(buf);
    //ESP_LOGE("FOBAUTH", "Fob receive done, authenticating");

    bdConvFromOctets(x, buf, RSA_SIZE_BYTES);
    bdModExp(y, x, PUBLIC_EXPONENT, MODULUS);

    //ESP_LOGE("FOBAUTH", "RSA verify done");

    return bdIsEqual(y, yp);
}