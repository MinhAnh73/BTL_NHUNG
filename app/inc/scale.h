#ifndef SCALE_H
#define SCALE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
 int32_t start_zero_raw;  /* raw at startup, for independent physical overload check */
 int32_t tare_raw;        /* raw saved on TARE */
 float counts_per_gram;
 float filtered_g;
 bool seeded;
} Scale_t;
void Scale_Init(Scale_t *s,int32_t start_raw,float factor);
void Scale_Tare(Scale_t *s,int32_t raw);
bool Scale_CalibrateKnownMass(Scale_t *s,int32_t raw,uint32_t known_mass_g);
int32_t Scale_Convert(Scale_t *s,int32_t raw);
int32_t Scale_Gross(const Scale_t *s,int32_t raw);
#endif
