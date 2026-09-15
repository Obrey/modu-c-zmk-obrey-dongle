/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <string.h>
#include "../local-modules/modu-dongle/src/battery_telemetry_protocol.h"
int main(void) {
    struct modu_battery_detail d={.side=1,.result=MODU_BAT_OK,.percent=0,.millivolts=3300,.age_seconds=4,.sequence=23};
    uint8_t p[12];struct modu_battery_detail out;char text[16];
    modu_battery_encode(&d,p);assert(modu_battery_decode(p,12,&out));
    assert(out.side==1 && out.percent==0 && out.millivolts==3300 && out.sequence==23);
    assert(!modu_battery_decode(p,11,&out));p[0]=2;assert(!modu_battery_decode(p,12,&out));p[0]=1;
    p[1]=2;assert(!modu_battery_decode(p,12,&out));p[1]=1;
    p[3]=255;assert(!modu_battery_decode(p,12,&out));
    modu_battery_format(text,sizeof(text),'L',NULL,false);assert(!strcmp(text,"L  --%"));
    modu_battery_format(text,sizeof(text),'L',&d,false);assert(!strcmp(text,"L   0%"));
    modu_battery_format(text,sizeof(text),'L',&d,true);assert(!strcmp(text,"L 3.30V"));
    d.millivolts=1900;modu_battery_format(text,sizeof(text),'R',&d,false);assert(!strcmp(text,"R ADC?"));
    modu_battery_format(text,sizeof(text),'R',&d,true);assert(!strcmp(text,"R 1.90V"));
    d.millivolts=4200;d.percent=100;modu_battery_format(text,sizeof(text),'R',&d,false);assert(!strcmp(text,"R 100%"));
    d.result=MODU_BAT_FETCH_ERROR;d.error=-5;d.percent=255;
    modu_battery_encode(&d,p);assert(modu_battery_decode(p,12,&out) && out.error==-5);
    modu_battery_format(text,sizeof(text),'L',&d,false);assert(!strcmp(text,"L ERR"));
    d.age_seconds=101;modu_battery_format(text,sizeof(text),'L',&d,false);assert(!strcmp(text,"L OLD"));
    for(unsigned mv=0;mv<=65535;mv+=127) {
        d=(struct modu_battery_detail){.result=MODU_BAT_OK,.millivolts=mv,.percent=0};
        for(int phase=0;phase<2;phase++){modu_battery_format(text,sizeof(text),'L',&d,phase);assert(strlen(text)<=7);}
    }
    puts("PASS: packet validation, error/zero distinction, voltage and 56px label bounds");return 0;
}
