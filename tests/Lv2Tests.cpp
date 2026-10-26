#include "Dsp.h"
#include "Lv2Ports.h"
#include <lv2/core/lv2.h>
#include <lv2/atom/forge.h>
#include <lv2/time/time.h>
#include <lv2/urid/urid.h>
#include <dlfcn.h>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace atomiccluster;
namespace port=atomiccluster::lv2port;
static void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
struct Host {
    std::map<std::string,LV2_URID> urids;
    static LV2_URID mapURI(void* handle,const char* uri) {
        auto& m=static_cast<Host*>(handle)->urids;
        auto found=m.find(uri); if(found!=m.end()) return found->second;
        const auto id=LV2_URID(m.size()+1); m.emplace(uri,id); return id;
    }
    LV2_URID_Map map{this,mapURI};
    LV2_Feature feature{LV2_URID__map,&map};
    const LV2_Feature* features[2]{&feature,nullptr};
    const LV2_Descriptor* descriptor; LV2_Handle instance;
    std::array<float,port::Count> control{};
    std::array<float,128> left{},right{},outL{},outR{};
    alignas(8) std::array<uint8_t,4096> events{};
    LV2_Atom_Forge forge{}; LV2_Atom_Forge_Frame sequence{};
    explicit Host(const LV2_Descriptor* d):descriptor(d),instance(d->instantiate(d,48000,".",features)) {
        check(instance,"instantiate real LV2 binary");
        control[port::Volume]=0; control[port::Blend]=100; control[port::SpeedMs]=float(interval(.4)*1000);
        control[port::Atoms]=16; control[port::Enabled]=1; control[port::Stereo]=100;
        control[port::Division]=2; control[port::Bpm]=120;
        for(int i=port::Volume;i<port::Count;++i) d->connect_port(instance,i,&control[i]);
        d->connect_port(instance,port::InputL,left.data()); d->connect_port(instance,port::InputR,right.data());
        d->connect_port(instance,port::OutputL,outL.data()); d->connect_port(instance,port::OutputR,outR.data());
        d->connect_port(instance,port::Events,nullptr); lv2_atom_forge_init(&forge,&map);
    }
    ~Host() { descriptor->cleanup(instance); }
    void activate() { descriptor->activate(instance); }
    void run() { descriptor->run(instance,128); check(control[port::Latency]==fftSize,"LV2 latency port"); }
    void beginEvents() { lv2_atom_forge_set_buffer(&forge,events.data(),events.size()); lv2_atom_forge_sequence_head(&forge,&sequence,0); }
    void position(int frame,double beat,double bpm) {
        LV2_Atom_Forge_Frame object{}; lv2_atom_forge_frame_time(&forge,frame);
        lv2_atom_forge_object(&forge,&object,0,mapURI(this,LV2_TIME__Position));
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__beatsPerMinute)); lv2_atom_forge_float(&forge,float(bpm));
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__speed)); lv2_atom_forge_float(&forge,1);
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__beat)); lv2_atom_forge_double(&forge,beat);
        lv2_atom_forge_pop(&forge,&object);
    }
    void barPosition(double bar,double offset,double bpm,int unit,int meter) {
        LV2_Atom_Forge_Frame object{}; lv2_atom_forge_frame_time(&forge,0);
        lv2_atom_forge_object(&forge,&object,0,mapURI(this,LV2_TIME__Position));
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__beatsPerMinute)); lv2_atom_forge_double(&forge,bpm);
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__speed)); lv2_atom_forge_float(&forge,1);
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__bar)); lv2_atom_forge_long(&forge,int64_t(bar));
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__barBeat)); lv2_atom_forge_float(&forge,float(offset));
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__beatUnit)); lv2_atom_forge_int(&forge,unit);
        lv2_atom_forge_key(&forge,mapURI(this,LV2_TIME__beatsPerBar)); lv2_atom_forge_float(&forge,float(meter));
        lv2_atom_forge_pop(&forge,&object);
    }
    void endEvents() { lv2_atom_forge_pop(&forge,&sequence); descriptor->connect_port(instance,port::Events,events.data()); }
};
static double tone(const std::vector<double>& x,double f) {
    std::complex<double> sum{}; for(size_t i=x.size()/2;i<x.size();++i) sum+=x[i]*std::polar(1.,-2*pi*f*i/48000.);
    return std::abs(sum)*4/x.size();
}
int main(int argc,char** argv) try {
    check(argc==2,"usage: lv2_tests path/to/atomic_cluster.so");
    auto* module=dlopen(argv[1],RTLD_NOW); if(!module) throw std::runtime_error(dlerror());
    auto descriptorFunction=reinterpret_cast<const LV2_Descriptor*(*)(uint32_t)>(dlsym(module,"lv2_descriptor"));
    check(descriptorFunction,"LV2 descriptor export"); const auto* d=descriptorFunction(0);
    check(d && !std::strcmp(d->URI,port::uri) && !descriptorFunction(1),"LV2 URI and descriptor count");
    check(!d->instantiate(d,48000,".",nullptr),"required URID feature must be enforced");
    int passed=1;
    for(bool bypass:{false,true}) {
        Host h(d); h.control[port::Blend]=bypass ? 100:0; h.control[port::Enabled]=bypass ? 0:1; h.activate();
        for(int pos=0;pos<8192;pos+=128) {
            for(int i=0;i<128;++i) { h.left[i]=float(.15*std::sin(.017*(pos+i))); h.right[i]=-h.left[i]; }
            h.run();
            for(int i=0;i<128;++i) {
                const float expected=pos+i>=fftSize ? float(.15*std::sin(.017*(pos+i-fftSize))):0;
                check(std::abs(h.outL[i]-expected)<1e-7 && std::abs(h.outR[i]+expected)<1e-7,"dry/bypass exact stereo with reported delay");
            }
        }
        ++passed;
    }
    {
        Host h(d); h.control[port::Smooth]=1; h.control[port::Atoms]=32;
        d->connect_port(h.instance,port::InputR,nullptr); h.activate(); double side=0,energy=0;
        for(int pos=0;pos<76800;pos+=128) {
            for(int i=0;i<128;++i) h.left[i]=float(.15*std::sin(2*pi*220*(pos+i)/48000.));
            h.run(); for(int i=0;i<128;++i) { side+=std::pow(h.outL[i]-h.outR[i],2); energy+=std::pow(h.outL[i],2)+std::pow(h.outR[i],2); }
        }
        check(energy>10 && side/energy>.1,"optional right input must duplicate mono into moving stereo"); ++passed;
    }
    {
        Host h(d); h.control[port::Smooth]=1; h.control[port::Atoms]=1; h.control[port::Stereo]=0; h.control[port::SpeedMs]=3;
        h.activate(); std::vector<double> y;
        for(int pos=0;pos<96000;pos+=128) {
            for(int i=0;i<128;++i) h.left[i]=h.right[i]=float(.2*std::sin(2*pi*220*(pos+i)/48000.)+.06*std::sin(2*pi*17000*(pos+i)/48000.));
            h.run(); for(auto sample:h.outL) y.push_back(sample);
        }
        check(tone(y,220)>.12 && tone(y,17000)<1e-4,"LV2 full wet must exclude unselected dry tone"); ++passed;
    }
    {
        Host h(d); h.control[port::Sync]=1; h.control[port::Division]=0; h.control[port::Atoms]=1; h.control[port::Stereo]=0;
        h.activate(); int gates=0; double energy=0;
        for(int pos=0;pos<96000;pos+=128) {
            h.beginEvents(); h.position(0,.375+pos/24000.,120); h.endEvents();
            for(int i=0;i<128;++i) h.left[i]=h.right[i]=float(.15*std::sin(2*pi*220*(pos+i)/48000.));
            h.run(); for(int i=0;i<128;++i) {
                energy+=h.outL[i]*h.outL[i];
                if(pos+i>12000 && (pos+i-fftSize-3000)%6000==0) { check(std::abs(h.outL[i])<1e-6,"LV2 PPQ pulse grid and latency"); ++gates; }
            }
        }
        check(gates>10 && energy>10,"LV2 host time events must drive audible beat grid"); ++passed;
    }
    {
        Host a(d),b(d);
        for(Host* h:{&a,&b}) { h->control[port::Sync]=1; h->control[port::Division]=0; h->control[port::Atoms]=1; h->control[port::Stereo]=0; h->activate(); }
        for(int pos=0;pos<24064;pos+=128) {
            for(int i=0;i<128;++i) a.left[i]=a.right[i]=b.left[i]=b.right[i]=float(.15*std::sin(2*pi*220*(pos+i)/48000.));
            a.run(); b.run();
        }
        for(Host* h:{&a,&b}) { h->beginEvents(); h->position(0,.4,120); if(h==&b) h->position(64,1+fftSize*90/(60*48000.),90); h->endEvents(); }
        a.run(); b.run(); double difference=0;
        for(int i=0;i<128;++i) { if(i<64) check(a.outL[i]==b.outL[i],"future atom must not apply before its sample offset"); else difference+=std::abs(a.outL[i]-b.outL[i]); }
        check(difference>.001,"sample-offset tempo/seek atom must change subsequent output"); ++passed;
    }
    {
        Host h(d); h.control[port::Blend]=NAN; h.control[port::Atoms]=INFINITY; h.activate();
        h.beginEvents(); h.endEvents();
        auto* seq=reinterpret_cast<LV2_Atom_Sequence*>(h.events.data());
        seq->atom.size=sizeof(LV2_Atom_Sequence_Body)+sizeof(LV2_Atom_Event);
        auto* event=reinterpret_cast<LV2_Atom_Event*>(h.events.data()+sizeof(LV2_Atom_Sequence));
        event->time.frames=0; event->body.type=1; event->body.size=1000;
        h.run(); for(auto x:h.outL) check(std::isfinite(x),"invalid control/atom input must remain finite"); ++passed;
    }
    {
        Host h(d); h.control[port::Blend]=0;
        d->connect_port(h.instance,port::OutputL,h.left.data()); d->connect_port(h.instance,port::OutputR,h.right.data()); h.activate();
        for(int pos=0;pos<8192;pos+=128) {
            for(int i=0;i<128;++i) { h.left[i]=float(.15*std::sin(.017*(pos+i))); h.right[i]=-h.left[i]; }
            h.run(); for(int i=0;i<128;++i) {
                const float expected=pos+i>=fftSize ? float(.15*std::sin(.017*(pos+i-fftSize))):0;
                check(std::abs(h.left[i]-expected)<1e-7 && std::abs(h.right[i]+expected)<1e-7,"in-place LV2 port processing");
            }
        }
        ++passed;
    }
    {
        // A 6/8 host at 120 eighth-notes/min equals 60 quarter-notes/min.
        Host a(d),b(d);
        for(Host* h:{&a,&b}) { h->control[port::Sync]=1; h->control[port::Division]=0; h->control[port::Atoms]=1; h->control[port::Stereo]=0; h->activate(); }
        a.beginEvents(); a.position(0,10.25,60); a.endEvents();
        b.beginEvents(); b.barPosition(3,2.5,120,8,6); b.endEvents();
        reinterpret_cast<LV2_Atom_Sequence*>(b.events.data())->body.unit=Host::mapURI(&b,LV2_TIME__frame);
        for(int block=0;block<500;++block) {
            if(block) { a.beginEvents();a.endEvents();b.beginEvents();b.endEvents(); }
            for(int i=0;i<128;++i) a.left[i]=a.right[i]=b.left[i]=b.right[i]=float(.15*std::sin(2*pi*220*(block*128+i)/48000.));
            a.run();b.run();
            for(int i=0;i<128;++i) check(std::abs(a.outL[i]-b.outL[i])<1e-6,"bar/beat-unit meter conversion must match absolute quarter-note clock");
        }
        ++passed;
    }
    dlclose(module); std::cout<<passed<<" LV2 host checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
