#ifndef MULAWENCODER_H
#define MULAWENCODER_H

#include <stdint.h>
#include <stddef.h>

uint8_t muLawEncode(int pcm);
uint8_t muLawEncode(short pcm);
uint8_t *muLawEncode(int *data, size_t size);
uint8_t *muLawEncode(short *data, size_t size);
uint8_t *muLawEncode(uint8_t *data, size_t size);
void muLawEncode(uint8_t *data, uint8_t *target, size_t t_size);
void muLawEncode(short* data, uint8_t *target, size_t size);

void constructEncoder();
void destructEncoder();

#endif // MULAWENCODER_H
