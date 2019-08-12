#include "mulawdecoder.h"
#include <limits>

static const int BIAS = 0x84;
static short * muLawToPcmMap;
static bool isConstructed = false;


static short decode(uint8_t mulaw){
    mulaw = ~mulaw;

    int sign = mulaw & 0x80;
    int exponent = (mulaw & 0x70) >> 4;
    int data = mulaw & 0x0f;

    data |= 0x10;
    data <<= 1;
    data += 1;
    data <<= exponent + 2;
    data -= BIAS;

    return static_cast<short>(sign == 0 ? data : -data);
}

short muLawDecode(uint8_t mulaw){
    return muLawToPcmMap[mulaw];
}

short *muLawDecode(uint8_t *data, size_t size){
    short *decoded = new short[size];
    for (size_t i = 0; i < size; ++i)
        decoded[i] = muLawToPcmMap[data[i]];
    return decoded;
}

void muLawDecode(uint8_t *data, short *decoded, size_t size){
    for (size_t i = 0; i < size; ++i)
        decoded[i] = muLawToPcmMap[data[i]];
}

void muLawDecode(uint8_t *data, uint8_t *decoded, size_t size){
    decoded = new uint8_t[size * 2];
    for(size_t i = 0; i < size; ++i){
        decoded[2 * i] = static_cast<uint8_t>(muLawToPcmMap[data[i]] & 0xff);
        decoded[2 * i + 1] = static_cast<uint8_t>(muLawToPcmMap[data[i]] >> 8);
    }
}

void constructDecoder(){
	if (!isConstructed) {
		muLawToPcmMap = new short[256];
		for (uint8_t i = 0; i < std::numeric_limits<uint8_t>::max(); ++i)
			muLawToPcmMap[i] = decode(i);
		isConstructed = true;
	}
}

void destructDecoder(){
	if (isConstructed) {
		delete muLawToPcmMap;
		isConstructed = false;
	}
}
