/**
 * @brief This library is for the A4988 Stepper Motor Driver
 * 
 *        Datasheet: https://www.pololu.com/file/0J450/a4988_DMOS_microstepping_driver_with_translator.pdf
 * 
 */
#pragma once

#include "BLE.h"
#include "bigd.h"
#include "bigdRand.h"

#define RSA_SIZE_BYTES 64

//#define RSA_PUBKEY_MODULUS "c38dec5ff37fbee973cc03e73d13757c537ad43a3ea41ced20193df66c337a4e3926d6d6e1af6e0738c2008fcde0a71ad9d22d394f8184c11116f68e719d96bd"
//#define RSA_PUBKEY_EXPONENT "10001"

class FobAuth {
public:
    FobAuth(const char* pubkeymod, const char* pubkeyexp);

    /*
    Assumes BLE is already initialized
    */
    bool doAuth(BLE& ble);
private:
    BIGD MODULUS, PUBLIC_EXPONENT, x, y, yp;
};

