#include "Dsp.h"
#include <iostream>
#include <random>
#include <vector>
#include <stdexcept>
using namespace atomiccluster;
static void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
static double tone(const std::vector<double>& x,double f,double sr=48000) {
    std::complex<double> sum{};
    for(size_t i=x.size()/2;i<x.size();++i) sum+=x[i]*std::polar(1.,-2*pi*f*i/sr);
    return std::abs(sum)*4/x.size();
}
static double rms(const std::vector<double>& x) {
    double e=0; for(size_t i=x.size()/2;i<x.size();++i) e+=x[i]*x[i];
    return std::sqrt(e/(x.size()-x.size()/2));
}
static std::vector<double> render(Values p,double sr=48000,bool noise=false,bool side=false) {
    Engine e; e.reset(sr,p); std::vector<double> out(int(sr*1.2));
    std::mt19937 rng(42); std::uniform_real_distribution<double> random(-.2,.2);
    for(int i=0;i<int(out.size());++i) {
        double x=noise ? random(rng) : .15*std::sin(2*pi*220*i/sr);
        auto y=e.process(x,x,p); out[i]=side ? y[0]-y[1]:y[0];
        check(std::isfinite(y[0]) && std::isfinite(y[1]) && std::abs(y[0])<2 && std::abs(y[1])<2,"finite bounded output");
    }
    return out;
}
int main() try {
    int passed=0;
    std::array<std::complex<double>,fftSize> a{},original{};
    for(int i=0;i<fftSize;++i) a[i]=original[i]={std::sin(i*.1),std::cos(i*.23)};
    fft(a); fft(a,true);
    for(int i=0;i<fftSize;++i) check(std::abs(a[i]-original[i])<1e-10,"FFT roundtrip");
    ++passed;
    // Dry and bypass preserve the original stereo waveform and declared delay.
    for(int bypass:{0,1}) {
        auto p=defaults(); p[Blend]=bypass ? 1:0; p[Bypass]=bypass;
        Engine e; e.reset(48000,p);
        for(int i=0;i<12000;++i) {
            double x=.25*std::sin(.017*i); auto y=e.process(x,-x,p);
            double expected=i>=fftSize ? .25*std::sin(.017*(i-fftSize)):0;
            check(std::abs(y[0]-expected)<1e-12 && std::abs(y[1]+expected)<1e-12,"dry/bypass stereo and latency");
        }
        ++passed;
    }
    // At 100% wet an unselected high input tone must not leak onto the bus.
    // Also exercise an endpoint automation from a previously 40% wet mix.
    {
        auto p=defaults(); p[Blend]=.4; p[Atoms]=p[Stereo]=0; p[Smooth]=p[Speed]=1;
        Engine e; e.reset(48000,p); std::vector<double> y(96000);
        for(int i=0;i<96000;++i) {
            if(i==24000) p[Blend]=1;
            double x=.2*std::sin(2*pi*220*i/48000)+.06*std::sin(2*pi*17000*i/48000);
            y[i]=e.process(x,x,p)[0];
        }
        check(tone(y,220)>.12 && tone(y,17000)<1e-4,"100% wet must exclude unselected dry input"); ++passed;
    }
    // A pure input sine still yields a denser harmonic effect as ATOMS grows.
    std::array<double,3> densityRms{}; int prior=0;
    for(int n:{1,16,64}) {
        auto p=defaults(); p[Blend]=p[Smooth]=p[Speed]=1; p[Stereo]=0;
        p[Atoms]=std::log(double(n))/std::log(64.);
        auto y=render(p); int audible=0;
        for(int harmonic=1;harmonic<=64;++harmonic) if(tone(y,220*harmonic)>.003) ++audible;
        check(audible>=n*.7 && audible>prior,"more ATOMS must yield more audible harmonic components");
        prior=audible; densityRms[n==1 ? 0:n==16 ? 1:2]=rms(y);
        std::cout<<"ATOMS="<<n<<": "<<audible<<" audible harmonics, RMS="<<rms(y)<<'\n'; ++passed;
    }
    check(densityRms[1]>.65*densityRms[0] && densityRms[2]>.65*densityRms[0],"dense atoms must not disappear in level"); ++passed;
    {
        auto p=defaults(); p[Blend]=p[Smooth]=p[Speed]=1; p[Stereo]=0;
        check(rms(render(p,48000,false,true))==0,"stereo zero must give identical mono wet channels");
        p[Stereo]=1;
        check(rms(render(p,48000,false,true))>.02,"mono input must create moving stereo atoms"); ++passed;
    }
    {
        auto p=defaults(); p[Blend]=p[Smooth]=1; p[Speed]=.3;
        p[Motion]=.5; auto arp=render(p);
        p[Motion]=1; auto random=render(p), again=render(p);
        check(random==again,"random harmonic mode must reset reproducibly");
        double difference=0; for(size_t i=0;i<arp.size();++i) difference+=std::abs(arp[i]-random[i]);
        check(difference>100,"arpeggio and random must create different patterns"); ++passed;
    }
    // Constant pitch/input isolates rhythmic SHARP vs uninterrupted SMOOTH.
    {
        std::array<double,2> spread{};
        for(int mode:{0,1}) {
            auto p=defaults(); p[Blend]=1; p[Atoms]=p[Stereo]=0; p[Smooth]=mode;
            p[Speed]=std::log(.2/2.)/std::log(1./750.);
            auto y=render(p); double low=10,high=0;
            for(size_t start=30000;start+480<y.size();start+=480) {
                double e=0; for(size_t i=start;i<start+480;++i) e+=y[i]*y[i];
                double level=std::sqrt(e/480); low=std::min(low,level); high=std::max(high,level);
            }
            spread[mode]=low/high;
        }
        check(spread[0]<.6 && spread[1]>.8,"SHARP must pulse while SMOOTH sustains"); ++passed;
    }
    for(double sr:{8000.,44100.,96000.,384000.}) for(int mode:{0,1}) {
        auto p=defaults(); p[Flat]=p[Blend]=p[Atoms]=1; p[Smooth]=mode; p[Motion]=mode;
        render(p,sr,true); ++passed;
    }
    {
        auto p=defaults(); p[Flat]=p[Blend]=1; Engine e; e.reset(48000,p);
        for(int i=0;i<12000;++i) check(e.process(0,0,p)[0]==0,"silence cannot self-oscillate");
        e.process(NAN,INFINITY,p);
        for(int i=0;i<5000;++i) { auto y=e.process(.1,-.1,p); check(std::isfinite(y[0]) && std::isfinite(y[1]),"invalid-input recovery"); }
        ++passed;
    }
    // A guitar played after activation must not wait for a two-second capture.
    {
        auto p=defaults(); p[Blend]=p[Smooth]=1; p[Speed]=0;
        Engine e; e.reset(48000,p); double energy=0;
        for(int i=0;i<62400;++i) {
            double x=i>=48000 ? .15*std::sin(2*pi*220*i/48000):0;
            auto y=e.process(x,x,p); if(i>=57600) energy+=y[0]*y[0];
        }
        check(energy>1,"late input onset must excite an empty held patch promptly"); ++passed;
    }
    {
        auto p=defaults(); p[Blend]=p[Smooth]=1; p[Stereo]=p[Atoms]=0;
        p[Speed]=std::log(.5/2.)/std::log(1./750.);
        Engine e; e.reset(48000,p); std::vector<double> tail;
        for(int i=0;i<96000;++i) {
            const double x=i<52800 ? .2*std::sin(2*pi*440*i/48000):0;
            auto y=e.process(x,x,p); if(i>=76800) tail.push_back(y[0]);
        }
        double best=0,frequency=0;
        for(double f=435;f<=450;f+=.25) if(tone(tail,f)>best) { best=tone(tail,f); frequency=f; }
        check(best>1e-5 && std::abs(frequency-440)<1.5,"released SMOOTH tone must retain captured pitch"); ++passed;
    }
    {
        auto p=defaults(); p[Blend]=p[Smooth]=p[Flat]=p[Speed]=1; p[Stereo]=0;
        p[Atoms]=std::log(2.)/std::log(64.); Engine e; e.reset(48000,p);
        std::vector<double> y(57600);
        for(int i=0;i<int(y.size());++i) {
            double x=.2*std::sin(2*pi*220*i/48000)+.04*std::sin(2*pi*440*i/48000);
            y[i]=e.process(x,x,p)[0];
        }
        double ratio=tone(y,440)/tone(y,220);
        check(ratio>.85 && ratio<1.15,"FLAT must lift the quiet measured partial"); ++passed;
    }
    {
        auto p=defaults(); p[Blend]=p[Smooth]=p[Stereo]=1; p[Atoms]=0;
        Engine e; e.reset(48000,p); double earlyL=0,earlyR=0,lateL=0,lateR=0;
        for(int i=0;i<96000;++i) {
            double x=.15*std::sin(2*pi*220*i/48000); auto y=e.process(x,x,p);
            if(i>=24000 && i<36000) { earlyL+=y[0]*y[0]; earlyR+=y[1]*y[1]; }
            if(i>=72000 && i<84000) { lateL+=y[0]*y[0]; lateR+=y[1]*y[1]; }
        }
        check(std::abs(std::sqrt(earlyL/earlyR)-std::sqrt(lateL/lateR))>.1,"atom position must move over time"); ++passed;
    }
    // Anti-phase input must not cancel the analysis and silence a wet patch.
    {
        auto p=defaults(); p[Blend]=p[Smooth]=1; Engine e; e.reset(48000,p); double energy=0;
        for(int i=0;i<48000;++i) {
            double x=.15*std::sin(2*pi*220*i/48000); auto y=e.process(x,-x,p);
            if(i>24000) energy+=y[0]*y[0]+y[1]*y[1];
        }
        check(energy>10,"anti-phase input must still excite atoms"); ++passed;
    }
    std::cout<<passed<<" DSP checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
