#include "Dsp.h"
#include "Lv2Ports.h"
#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/time/time.h>
#include <lv2/urid/urid.h>
#include <cstring>
#include <new>
#include <limits>
namespace atomiccluster {
namespace port=lv2port;
class Lv2Plugin {
    std::array<void*,port::Count> ports{};
    Engine engine;
    Values values=defaults();
    double rate=48000, tempo=120, hostTempo=120, beat=0, beatsPerBar=4, beatUnit=4;
    bool haveTempo=false, haveBeat=false, playing=false, tapDown=false, haveTap=false;
    uint64_t samples=0, lastTap=0;
    struct IDs { LV2_URID sequence,object,blank,floatType,doubleType,intType,longType,position,bpm,speed,beat,barBeat,bar,beatsPerBar,beatUnit,frame; } ids{};
    double control(port::Port p,double fallback,double low,double high) const {
        const auto* v=static_cast<const float*>(ports[p]);
        return v && std::isfinite(*v) ? std::clamp(double(*v),low,high):fallback;
    }
    static bool number(const LV2_Atom* a,const IDs& id,double& value) {
        if(!a) return false;
        if(a->type==id.floatType && a->size==sizeof(float)) { float v; std::memcpy(&v,a+1,sizeof(v)); value=v; }
        else if(a->type==id.doubleType && a->size==sizeof(double)) std::memcpy(&value,a+1,sizeof(value));
        else if(a->type==id.intType && a->size==sizeof(int32_t)) { int32_t v; std::memcpy(&v,a+1,sizeof(v)); value=v; }
        else if(a->type==id.longType && a->size==sizeof(int64_t)) { int64_t v; std::memcpy(&v,a+1,sizeof(v)); value=double(v); }
        else return false;
        return std::isfinite(value);
    }
    void position(const LV2_Atom_Event* event) {
        if((event->body.type!=ids.object && event->body.type!=ids.blank) || event->body.size<sizeof(LV2_Atom_Object_Body)) return;
        const auto* object=reinterpret_cast<const LV2_Atom_Object*>(&event->body);
        if(object->body.otype!=ids.position) return;
        // Validate property extents before using host-provided atom payloads.
        const uint8_t* p=reinterpret_cast<const uint8_t*>(&object->body)+sizeof(object->body);
        const uint8_t* end=reinterpret_cast<const uint8_t*>(&object->body)+object->atom.size;
        double absolute=0,barBeat=0,bar=0; bool absoluteValid=false,barBeatValid=false,barValid=false;
        while(size_t(end-p)>=sizeof(LV2_Atom_Property_Body)) {
            const auto* prop=reinterpret_cast<const LV2_Atom_Property_Body*>(p);
            const uint64_t bytes=uint64_t(sizeof(*prop))+prop->value.size;
            if(bytes>uint64_t(end-p)) return;
            double v=0;
            if(number(&prop->value,ids,v)) {
                if(prop->key==ids.bpm && v>=20 && v<=400) { hostTempo=v; haveTempo=true; }
                else if(prop->key==ids.speed) playing=v>0;
                else if(prop->key==ids.beat) { absolute=v; absoluteValid=true; }
                else if(prop->key==ids.barBeat) { barBeat=v; barBeatValid=true; }
                else if(prop->key==ids.bar) { bar=v; barValid=true; }
                else if(prop->key==ids.beatsPerBar && v>0 && v<=64) beatsPerBar=v;
                else if(prop->key==ids.beatUnit && v>=1 && v<=64) beatUnit=v;
            }
            const uint64_t padded=(bytes+7)&~uint64_t(7);
            if(padded>uint64_t(end-p)) break;
            p+=padded;
        }
        if(haveTempo) tempo=std::clamp(hostTempo*4/beatUnit,20.,400.);
        if(absoluteValid) { beat=absolute*4/beatUnit; haveBeat=true; }
        else if(barBeatValid && barValid) { beat=(bar*beatsPerBar+barBeat)*4/beatUnit; haveBeat=true; }
        else if(barBeatValid) { beat=barBeat*4/beatUnit; haveBeat=true; }
    }
public:
    Lv2Plugin(double sr,LV2_URID_Map& map):rate(sr) {
        auto u=[&](const char* uri) { return map.map(map.handle,uri); };
        ids={u(LV2_ATOM__Sequence),u(LV2_ATOM__Object),u(LV2_ATOM__Blank),u(LV2_ATOM__Float),u(LV2_ATOM__Double),u(LV2_ATOM__Int),u(LV2_ATOM__Long),u(LV2_TIME__Position),u(LV2_TIME__beatsPerMinute),u(LV2_TIME__speed),u(LV2_TIME__beat),u(LV2_TIME__barBeat),u(LV2_TIME__bar),u(LV2_TIME__beatsPerBar),u(LV2_TIME__beatUnit),u(LV2_TIME__frame)};
    }
    void connect(uint32_t index,void* data) { if(index<port::Count) ports[index]=data; }
    void activate() {
        // Match a host's restored controls on activation, before the first sample.
        readControls(); engine.reset(rate,values); samples=lastTap=0; tapDown=haveTap=false;
        haveTempo=haveBeat=playing=false; tempo=control(port::Bpm,120,20,400);
        hostTempo=tempo; beat=0; beatsPerBar=beatUnit=4;
    }
    void readControls() {
        values[Volume]=std::pow(10.,control(port::Volume,0,-60,6)/20.)/2;
        values[Blend]=control(port::Blend,100,0,100)/100;
        values[Speed]=clean(std::log(control(port::SpeedMs,interval(.4)*1000,2.666666,2000)/2000)/std::log(1./750.));
        values[Atoms]=std::log(std::round(control(port::Atoms,16,1,64)))/std::log(64.);
        values[Smooth]=control(port::Smooth,0,0,1)>=.5;
        values[Flat]=control(port::Flat,0,0,1)>=.5;
        values[Bypass]=control(port::Enabled,1,0,1)<.5;
        values[Sync]=control(port::Sync,0,0,1)>=.5;
        values[Division]=std::round(control(port::Division,2,0,4))/4;
        values[Motion]=std::round(control(port::Pattern,0,0,2))/2;
        values[Stereo]=control(port::Stereo,100,0,100)/100;
    }
    void run(uint32_t count) {
        readControls();
        if(!haveTempo) tempo=control(port::Bpm,120,20,400);
        const bool tap=control(port::Tap,0,0,1)>=.5;
        if(tap && !tapDown) {
            if(haveTap && samples>lastTap) {
                const double seconds=double(samples-lastTap)/rate;
                if(seconds>=.15 && seconds<=3) { tempo=std::clamp(60/seconds,20.,400.); haveTempo=true; }
            }
            lastTap=samples; haveTap=true;
        }
        tapDown=tap;
        if(auto* latency=static_cast<float*>(ports[port::Latency])) *latency=fftSize;
        const auto* sequence=static_cast<const LV2_Atom_Sequence*>(ports[port::Events]);
        const uint8_t* current=nullptr; const uint8_t* end=nullptr;
        if(sequence && sequence->atom.type==ids.sequence && sequence->atom.size>=sizeof(LV2_Atom_Sequence_Body) && (sequence->body.unit==0 || sequence->body.unit==ids.frame)) {
            current=reinterpret_cast<const uint8_t*>(&sequence->body)+sizeof(sequence->body);
            end=reinterpret_cast<const uint8_t*>(&sequence->body)+sequence->atom.size;
        }
        const auto* l=static_cast<const float*>(ports[port::InputL]);
        const auto* r=static_cast<const float*>(ports[port::InputR]);
        auto* ol=static_cast<float*>(ports[port::OutputL]); auto* oright=static_cast<float*>(ports[port::OutputR]);
        for(uint32_t i=0;i<count;++i) {
            while(current && size_t(end-current)>=sizeof(LV2_Atom_Event)) {
                const auto* event=reinterpret_cast<const LV2_Atom_Event*>(current);
                const uint64_t bytes=sizeof(LV2_Atom_Event)+uint64_t(event->body.size);
                if(bytes>uint64_t(end-current)) { current=nullptr; break; }
                if(event->time.frames>int64_t(i)) break;
                position(event);
                const uint64_t padded=(bytes+7)&~uint64_t(7);
                if(padded>uint64_t(end-current)) { current=nullptr; break; }
                current+=padded;
            }
            const double musicalBeat=haveBeat && playing && values[Sync]>=.5 ? beat-fftSize*tempo/(60*rate):NAN;
            // Read both before writing: audio ports may be connected in-place.
            const double left=l ? l[i]:0, right=r ? r[i]:left;
            const auto y=engine.process(left,right,values,tempo,musicalBeat);
            if(ol) ol[i]=float(y[0]); if(oright) oright[i]=float(y[1]);
            if(playing && haveBeat) beat+=tempo/(60*rate);
        }
        samples+=count;
    }
};
static LV2_Handle instantiate(const LV2_Descriptor*,double rate,const char*,const LV2_Feature* const* features) {
    if(!std::isfinite(rate) || rate<8000 || rate>384000) return nullptr;
    LV2_URID_Map* map=nullptr;
    if(features) for(auto f=features;*f;++f) if(!std::strcmp((*f)->URI,LV2_URID__map)) map=static_cast<LV2_URID_Map*>((*f)->data);
    if(!map || !map->map) return nullptr;
    return new(std::nothrow) Lv2Plugin(rate,*map);
}
static void connect(LV2_Handle p,uint32_t index,void* data) { static_cast<Lv2Plugin*>(p)->connect(index,data); }
static void activate(LV2_Handle p) { static_cast<Lv2Plugin*>(p)->activate(); }
static void run(LV2_Handle p,uint32_t n) { static_cast<Lv2Plugin*>(p)->run(n); }
static void cleanup(LV2_Handle p) { delete static_cast<Lv2Plugin*>(p); }
}
extern "C" LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    static const LV2_Descriptor descriptor{atomiccluster::lv2port::uri,atomiccluster::instantiate,atomiccluster::connect,atomiccluster::activate,atomiccluster::run,nullptr,atomiccluster::cleanup,nullptr};
    return index==0 ? &descriptor:nullptr;
}
