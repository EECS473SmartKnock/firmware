#include "FobAuth.h"

FobAuth::FobAuth(const char* pubkeymod, const char* pubkeyexp) {
  bdNewVars(&MODULUS, &PUBLIC_EXPONENT, &x, &y, &yp, NULL);
  bdConvFromHex(MODULUS, pubkeymod);
  bdConvFromHex(PUBLIC_EXPONENT, pubkeyexp);
}

bool FobAuth::doAuth(BLE& ble) {
    while (!ble.connectToFob());

    uint8_t buf[RSA_SIZE_BYTES]; //challenge string for fob
    bdRandomNumber(yp, MODULUS);
    bdConvToOctets(yp, buf, RSA_SIZE_BYTES);
    ble.fobWrite(buf);

    ble.fobRecv(buf);
    bdConvFromOctets(x, buf, RSA_SIZE_BYTES);
    
    bdModExp(y, x, PUBLIC_EXPONENT, MODULUS);
    return bdIsEqual(y, yp);
}