// Offline host for the actual compiled VST3, not a second DSP implementation.
#include "Plugin.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/common/memorystream.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif
using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace atomiccluster;
static void check(bool c, const char* s) { if (!c) throw std::runtime_error(s); }
template<class T> static T read(std::istream& f) { T x{}; f.read(reinterpret_cast<char*>(&x), sizeof(x)); check(bool(f), "truncated WAV"); return x; }
template<class T> static void write(std::ostream& f, T x) { f.write(reinterpret_cast<const char*>(&x), sizeof(x)); }
int main(int argc, char** argv) try {
    check(argc == 10, "render_audio plugin input.wav output.wav volume blend period_ms atoms smooth flat");
    std::ifstream file(argv[2], std::ios::binary); check(bool(file), "open source WAV");
    char tag[4]; file.read(tag, 4); check(std::memcmp(tag,"RIFF",4)==0,"RIFF input");
    read<uint32_t>(file); file.read(tag,4); check(std::memcmp(tag,"WAVE",4)==0,"WAVE input");
    uint16_t channels=0, bits=0, format=0; uint32_t sr=0; std::vector<int16_t> pcm;
    while(file.read(tag,4)) {
        auto bytes=read<uint32_t>(file); auto next=file.tellg()+std::streamoff(bytes+(bytes&1));
        if(std::memcmp(tag,"fmt ",4)==0) {
            format=read<uint16_t>(file); channels=read<uint16_t>(file); sr=read<uint32_t>(file);
            read<uint32_t>(file); read<uint16_t>(file); bits=read<uint16_t>(file);
        } else if(std::memcmp(tag,"data",4)==0) {
            check(bytes%2==0,"PCM byte count"); pcm.resize(bytes/2); file.read(reinterpret_cast<char*>(pcm.data()),bytes);
        }
        file.seekg(next);
    }
    check(format==1 && bits==16 && (channels==1 || channels==2) && sr>=8000 && !pcm.empty(),"16-bit mono/stereo PCM WAV required");
#if defined(_WIN32)
    auto module=LoadLibraryA(argv[1]); check(module,"load released Windows VST3");
    auto symbol=[&](const char* n) { return reinterpret_cast<void*>(GetProcAddress(module,n)); };
    auto enter=reinterpret_cast<bool(*)()>(symbol("InitDll"));
    auto leave=reinterpret_cast<bool(*)()>(symbol("ExitDll"));
    check(enter && leave && enter(),"Windows module initialization");
#else
    auto module=dlopen(argv[1],RTLD_NOW); check(module,"load VST3 shared module");
    auto symbol=[&](const char* n) { return dlsym(module,n); };
    auto enter=reinterpret_cast<bool(*)(void*)>(symbol("ModuleEntry"));
    auto leave=reinterpret_cast<bool(*)()>(symbol("ModuleExit"));
    check(enter && leave && enter(module),"Linux module initialization");
#endif
    auto factoryFn=reinterpret_cast<IPluginFactory*(*)()>(symbol("GetPluginFactory")); check(factoryFn,"factory export");
    auto* factory=factoryFn(); IComponent* component=nullptr; IAudioProcessor* processor=nullptr;
    check(factory->createInstance(processorId,IComponent::iid,reinterpret_cast<void**>(&component))==kResultOk,"processor instance");
    check(component->initialize(nullptr)==kResultOk,"component initialization");
    check(component->queryInterface(IAudioProcessor::iid,reinterpret_cast<void**>(&processor))==kResultOk,"audio processor interface");
    auto p=defaults(); p[Volume]=std::stod(argv[4]); p[Blend]=std::stod(argv[5]);
    p[Speed]=std::log(std::stod(argv[6])/2000.)/std::log(1./750.);
    p[Atoms]=std::log(std::stod(argv[7]))/std::log(1023.); p[Smooth]=std::stod(argv[8]); p[Flat]=std::stod(argv[9]);
    for(auto v:p) check(std::isfinite(v) && v>=0 && v<=1,"parameter range");
    MemoryStream state; IBStreamer s(&state,kLittleEndian); s.writeInt32(0x4A414331); s.writeInt32(1);
    for(auto v:p) s.writeDouble(v); state.seek(0,IBStream::kIBSeekSet,nullptr);
    check(component->setState(&state)==kResultOk,"restore requested preset");
    SpeakerArrangement layout=SpeakerArr::kStereo;
    check(processor->setBusArrangements(&layout,1,&layout,1)==kResultOk,"stereo buses");
    ProcessSetup setup{}; setup.sampleRate=sr; setup.maxSamplesPerBlock=256; setup.symbolicSampleSize=kSample32; setup.processMode=kOffline;
    check(processor->setupProcessing(setup)==kResultOk,"offline processing setup");
    component->activateBus(kAudio,kInput,0,true); component->activateBus(kAudio,kOutput,0,true);
    check(component->setActive(true)==kResultOk,"activate processor"); processor->setProcessing(true);
    const size_t latency=processor->getLatencySamples(), inputFrames=pcm.size()/channels;
    const size_t frames=inputFrames+sr*3, total=frames+latency;
    std::vector<float> result(frames*2); std::array<float,256> l{},r{},ol{},orr{};
    float* ins[]{l.data(),r.data()}; float* outs[]{ol.data(),orr.data()};
    AudioBusBuffers in{},out{}; in.numChannels=out.numChannels=2; in.channelBuffers32=ins; out.channelBuffers32=outs;
    ProcessData data{}; data.processMode=kOffline; data.symbolicSampleSize=kSample32; data.numInputs=data.numOutputs=1; data.inputs=&in; data.outputs=&out;
    double peak=0, energy=0;
    for(size_t pos=0;pos<total;pos+=256) {
        const int n=int(std::min(size_t(256),total-pos));
        for(int i=0;i<n;++i) {
            const auto f=pos+i; l[i]=f<inputFrames?pcm[f*channels]/32768.f:0;
            r[i]=f<inputFrames?pcm[f*channels+(channels==2?1:0)]/32768.f:0;
        }
        in.silenceFlags=0; out.silenceFlags=0; data.numSamples=n;
        check(processor->process(data)==kResultOk,"render VST3 audio");
        for(int i=0;i<n;++i) if(pos+i>=latency) {
            const auto f=pos+i-latency;
            for(int c=0;c<2;++c) {
                const double x=c?orr[i]:ol[i]; check(std::isfinite(x),"non-finite plugin output");
                check(std::abs(x)<.999,"output clips; lower common input level");
                peak=std::max(peak,std::abs(x)); energy+=x*x; result[f*2+c]=float(x);
            }
        }
    }
    check(energy>1e-6,"unexpected silent render");
    std::ofstream dst(argv[3],std::ios::binary); check(bool(dst),"open output WAV");
    uint32_t bytes=uint32_t(result.size()*2); dst.write("RIFF",4); write(dst,uint32_t(36+bytes)); dst.write("WAVEfmt ",8);
    write(dst,uint32_t(16)); write(dst,uint16_t(1)); write(dst,uint16_t(2)); write(dst,sr); write(dst,uint32_t(sr*4));
    write(dst,uint16_t(4)); write(dst,uint16_t(16)); dst.write("data",4); write(dst,bytes);
    for(auto x:result) write(dst,int16_t(std::lround(x*32767))); check(bool(dst),"write output");
    processor->setProcessing(false); component->setActive(false); processor->release(); component->terminate(); component->release(); factory->release(); leave();
#if defined(_WIN32)
    FreeLibrary(module);
#else
    dlclose(module);
#endif
    std::cout << argv[3] << ": " << frames << " frames, peak=" << peak << ", RMS=" << std::sqrt(energy/result.size()) << ", compensated latency=" << latency << '\n';
    return 0;
} catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
