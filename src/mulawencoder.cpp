#include "mulawencoder.h"
#include <limits>

static const int BIAS = 0x84;
static const int MAX = 32635;
static uint8_t * pcmToMuLawMap;
static bool isConstructed = false;


static uint8_t encode(int pcm){
    int sign = (pcm & 0x8000) >> 8;

    if (sign != 0)
        pcm = -pcm;
    if (pcm > MAX)
        pcm = MAX;
    pcm += BIAS;

    int exponent = 7;
    for (int expMask = 0x4000; (pcm & expMask) == 0; exponent--, expMask>>=1)
        ;
    int mantisa = (pcm >> (exponent + 3)) & 0x0f;
    uint8_t mulaw = static_cast<uint8_t>(sign | exponent << 4 | mantisa);

    return ~mulaw;
}

uint8_t muLawEncode(int pcm){
    return pcmToMuLawMap[pcm & 0xffff];
}

uint8_t muLawEncode(short pcm){
    return pcmToMuLawMap[pcm & 0xffff];
}

uint8_t *muLawEncode(int *data, size_t size){
    uint8_t *encoded = new uint8_t[size];
    for (size_t i = 0; i < size; ++i)
        encoded[i] = muLawEncode(data[i]);
    return encoded;
}

uint8_t *muLawEncode(short *data, size_t size){
    uint8_t *encoded = new uint8_t[size];
    for (size_t i = 0; i < size; ++i)
        encoded[i] = muLawEncode(data[i]);
    return encoded;
}

uint8_t *muLawEncode(uint8_t *data, size_t size){
    uint8_t *encoded = new uint8_t[size];
    for (size_t i = 0; i < size; ++i)
        encoded[i] = muLawEncode((data[2 * i + 1] << 8) | data[2 * i]);
    return encoded;
}

void muLawEncode(uint8_t *data, uint8_t *target, size_t t_size){
    for (size_t i = 0; i < t_size; ++i)
        target[i] = muLawEncode((data[2 * i + 1] << 8) | data[2 * i]);
}

void muLawEncode(short * data, uint8_t * target, size_t size) {
	for (size_t i = 0; i < size; ++i)
		target[i] = muLawEncode(data[i]);
}

void constructEncoder(){
	if (!isConstructed) {
		pcmToMuLawMap = new uint8_t[65536];
		for (int i = std::numeric_limits<short>::min(); i <= std::numeric_limits<short>::max(); ++i)
			pcmToMuLawMap[(i & 0xffff)] = encode(i);
		isConstructed = true;
	}
}

void destructEncoder(){
	if (isConstructed) {
		delete[] pcmToMuLawMap;
		isConstructed = false;
	}
}
