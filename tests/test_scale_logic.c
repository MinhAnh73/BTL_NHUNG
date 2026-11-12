#include <assert.h>
#include <stdio.h>
#include "scale.h"
int main(void){
 Scale_t s;Scale_Init(&s,1000000,1000.0f);
 assert(Scale_Gross(&s,1000000)==0);
 assert(Scale_Convert(&s,1000000)==0);
 assert(Scale_CalibrateKnownMass(&s,1500000,1000)); /* exactly 1000g reference */
 assert(Scale_Gross(&s,1500000)==1000);
 assert(Scale_Convert(&s,1500000)==1000);
 Scale_Tare(&s,1500000); /* bowl is now zero */
 assert(Scale_Convert(&s,1500000)==0);
 assert(Scale_Gross(&s,1500000)==1000); /* physical load still present! */
 assert(Scale_Convert(&s,1600000)==70); /* first EMA step: alpha=0.35 */
 for(int i=0;i<20;i++)(void)Scale_Convert(&s,1600000);
 assert(Scale_Convert(&s,1600000)==200);
 assert(!Scale_CalibrateKnownMass(&s,1600000,0));
 Scale_Init(&s,1000000,1000.0f);
 assert(Scale_CalibrateKnownMass(&s,1500000,2000));
 assert(Scale_Gross(&s,1500000)==2000);
 puts("Scale tare/calibration tests: PASS");return 0;
}
