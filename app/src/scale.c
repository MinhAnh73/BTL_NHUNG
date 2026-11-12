#include "scale.h"
#include "app_config.h"
#include <math.h>
static int32_t round_int(float f){return (int32_t)(f >= 0.0f ? f+0.5f : f-0.5f);}
void Scale_Init(Scale_t *s,int32_t raw,float factor){
 s->start_zero_raw=raw;s->tare_raw=raw;
 s->counts_per_gram=(isfinite(factor)&&fabsf(factor)>1.0f)?factor:DEFAULT_COUNTS_PER_GRAM;
 s->filtered_g=0.0f;s->seeded=false;
}
void Scale_Tare(Scale_t *s,int32_t raw){s->tare_raw=raw;s->filtered_g=0;s->seeded=false;}
bool Scale_CalibrateKnownMass(Scale_t *s,int32_t raw,uint32_t known_mass_g){
 if(known_mass_g<CAL_MIN_G||known_mass_g>CAL_MAX_G)return false;
 /* A known mass establishes counts-per-gram; zero RAW captured with TARE. */
 float factor=((float)raw-(float)s->tare_raw)/(float)known_mass_g;
 if(!isfinite(factor)||fabsf(factor)<2.0f||fabsf(factor)>500000.0f)return false;
 s->counts_per_gram=factor;s->filtered_g=0;s->seeded=false;return true;
}
int32_t Scale_Convert(Scale_t *s,int32_t raw){
 float grams=((float)raw-(float)s->tare_raw)/s->counts_per_gram;
 if(!s->seeded){s->filtered_g=grams;s->seeded=true;}
 else s->filtered_g=0.35f*grams+0.65f*s->filtered_g;
 return round_int(s->filtered_g);
}
int32_t Scale_Gross(const Scale_t *s,int32_t raw){
 return round_int(((float)raw-(float)s->start_zero_raw)/s->counts_per_gram);
}
