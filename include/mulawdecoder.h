#ifndef MULAWDECODER_H
#define MULAWDECODER_H

#include <stdint.h>
#include <stddef.h>

short muLawDecode(uint8_t mulaw);
short *muLawDecode(uint8_t *data, size_t size);
void muLawDecode(uint8_t *data, short *decoded, size_t size);
void muLawDecode(uint8_t *data, uint8_t *decoded, size_t size);

void constructDecoder();
void destructDecoder();

#endif // MULAWDECODER_H
