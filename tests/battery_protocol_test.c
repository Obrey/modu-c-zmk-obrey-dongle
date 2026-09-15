/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <string.h>
#include "../local-modules/modu-dongle/src/battery_telemetry_protocol.h"
int main(void) {
    struct modu_battery_detail d={.side=1,.result=MODU_BAT_OK,.percent=0,.millivolts=3320,.age_seconds=4,.sequence=23};
    uint8_t p[MODU_BATTERY_PACKET_SIZE];struct modu_battery_detail out;char text[16];
    modu_battery_encode(&d,p);assert(modu_battery_decode(p,sizeof(p),&out));
    assert(out.side==1 && out.percent==0 && out.millivolts==3320 && out.sequence==23 && out.flags==0);
    assert(!modu_battery_decode(p,11,&out));p[0]=3;assert(!modu_battery_decode(p,sizeof(p),&out));p[0]=2;
    p[1]=2;assert(!modu_battery_decode(p,sizeof(p),&out));p[1]=1;
    p[3]=255;assert(!modu_battery_decode(p,sizeof(p),&out));p[3]=0;
    p[12]=2;assert(!modu_battery_decode(p,sizeof(p),&out));p[12]=0;
    p[13]=1;assert(!modu_battery_decode(p,sizeof(p),&out));p[13]=0;
    /* A new dongle still reads an old v6 hand. An old dongle must be upgraded. */
    p[0]=1;assert(modu_battery_decode(p,12,&out) && out.flags==0);
    assert(!modu_battery_decode(p,sizeof(p),&out));p[0]=2;
    modu_battery_format(text,sizeof(text),'L',NULL,false);assert(!strcmp(text,"L  --%"));
    modu_battery_format(text,sizeof(text),'L',&d,false);assert(!strcmp(text,"L   0%"));
    modu_battery_format(text,sizeof(text),'L',&d,true);assert(!strcmp(text,"L 3.32V"));
    d.millivolts=3330;modu_battery_format(text,sizeof(text),'R',&d,true);assert(!strcmp(text,"R 3.33V"));
    d.millivolts=1900;modu_battery_format(text,sizeof(text),'R',&d,false);assert(!strcmp(text,"R ADC?"));
    modu_battery_format(text,sizeof(text),'R',&d,true);assert(!strcmp(text,"R 1.90V"));
    d.millivolts=4200;d.percent=100;modu_battery_format(text,sizeof(text),'R',&d,false);assert(!strcmp(text,"R 100%"));
    modu_battery_format(text,sizeof(text),'R',&d,true);assert(!strcmp(text,"R 4.20V"));
    d.flags=MODU_BATTERY_FLAG_IDLE;d.age_seconds=300;
    modu_battery_format(text,sizeof(text),'R',&d,false);assert(!strcmp(text,"R~100%"));
    modu_battery_format(text,sizeof(text),'R',&d,true);assert(!strcmp(text,"R~4.20V"));
    d.age_seconds=901;modu_battery_format(text,sizeof(text),'R',&d,false);assert(!strcmp(text,"R OLD"));
    d.flags=0;d.age_seconds=0;d.result=MODU_BAT_FETCH_ERROR;d.error=-5;d.percent=255;
    modu_battery_encode(&d,p);assert(modu_battery_decode(p,sizeof(p),&out) && out.error==-5);
    modu_battery_format(text,sizeof(text),'L',&d,false);assert(!strcmp(text,"L ERR"));
    d.age_seconds=181;modu_battery_format(text,sizeof(text),'L',&d,false);assert(!strcmp(text,"L OLD"));
    for(unsigned mv=0;mv<=65535;mv+=127) {
        d=(struct modu_battery_detail){.result=MODU_BAT_OK,.millivolts=mv,.percent=0};
        for(int idle=0;idle<2;idle++) {d.flags=idle;
            for(int phase=0;phase<2;phase++){modu_battery_format(text,sizeof(text),'L',&d,phase);assert(strlen(text)<=7);}
        }
    }
    puts("PASS: v1/v2 packets, 3.32/3.33V zero preserved, cached/expired data and 56px label bounds");return 0;
}
