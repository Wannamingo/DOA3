#include "button_bindings.h"
#include <assert.h>
#include <stdio.h>

int main() {
    int map[16]={1,2,3,4,5,17,7,8,9,10,11,12,13,14,15,16};
    uint8_t raw[16]={};
    for(int i=0;i<16;++i){raw[i]=255;auto s=doa3::map_buttons(raw,map);if(i==5)assert(s.menu&&!s.digital);else if(i<8)assert(s.digital==(1u<<i));else assert(s.analog[i-8]==255);raw[i]=0;}
    map[8]=10;map[9]=9;raw[8]=255;auto s=doa3::map_buttons(raw,map);assert(s.analog[1]==255&&!s.analog[0]);
    raw[9]=255;s=doa3::map_buttons(raw,map);assert(s.analog[0]==255&&s.analog[1]==255);
    map[8]=15;map[14]=15;raw[14]=170;s=doa3::map_buttons(raw,map);assert(s.analog[6]==255);raw[8]=0;s=doa3::map_buttons(raw,map);assert(s.analog[6]==170);
    map[14]=5;raw[14]=127;s=doa3::map_buttons(raw,map);assert(!(s.digital&16));raw[14]=128;s=doa3::map_buttons(raw,map);assert(s.digital&16);
    map[9]=0;map[14]=99;s=doa3::map_buttons(raw,map);assert(!s.digital&&!s.menu);for(auto a:s.analog)assert(!a);
    assert(doa3::stick_directions(0,0)==0&&doa3::stick_directions(3000,3000)==0);
    assert(doa3::stick_directions(32767,0)==8&&doa3::stick_directions(-32768,0)==4);
    assert(doa3::stick_directions(0,32767)==1&&doa3::stick_directions(0,-32768)==2);
    assert(doa3::stick_directions(32767,32767)==9&&doa3::stick_directions(-32768,32767)==5);
    assert(doa3::stick_directions(32767,-32768)==10&&doa3::stick_directions(-32768,-32768)==6);
    puts("Native bindings passed: defaults, cycles, simultaneous presses, shared actions, analog pressure, threshold, disabled/invalid actions.");
}
