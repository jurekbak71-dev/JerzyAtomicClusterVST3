#include "Plugin.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "public.sdk/source/main/pluginfactory.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

namespace atomiccluster {
using namespace Steinberg;
using namespace Steinberg::Vst;
constexpr int32 stateMagic = 0x4A414331;
static bool readState(IBStream* stream, Values& v) {
    if (!stream) return false;
    IBStreamer s(stream, kLittleEndian);
    int32 magic = 0, version = 0;
    if (!s.readInt32(magic) || !s.readInt32(version) || magic != stateMagic || version != 1) return false;
    for (auto& value : v) if (!s.readDouble(value) || !std::isfinite(value) || value < 0 || value > 1) return false;
    return true;
}
Processor::Processor() {
    setControllerClass(controllerId);
    auto initial = defaults();
    for (int i = 0; i < NumParams; ++i) values[i].store(initial[i]);
    processContextRequirements.needTempo();
}
tresult PLUGIN_API Processor::initialize(FUnknown* host) {
    auto result = AudioEffect::initialize(host);
    if (result != kResultOk) return result;
    addAudioInput(STR16("Input"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Output"), SpeakerArr::kStereo);
    return kResultOk;
}
tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* ins, int32 ni, SpeakerArrangement* outs, int32 no) {
    if (ni != 1 || no != 1 || !ins || !outs || ins[0] != outs[0] ||
        (ins[0] != SpeakerArr::kMono && ins[0] != SpeakerArr::kStereo)) return kResultFalse;
    getAudioInput(0)->setArrangement(ins[0]);
    getAudioOutput(0)->setArrangement(outs[0]);
    return kResultOk;
}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 size) {
    return size == kSample32 || size == kSample64 ? kResultTrue : kResultFalse;
}
tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup) {
    if (!std::isfinite(setup.sampleRate) || setup.sampleRate < 8000 || setup.sampleRate > 384000 || setup.maxSamplesPerBlock < 0)
        return kResultFalse;
    return AudioEffect::setupProcessing(setup);
}
tresult PLUGIN_API Processor::setActive(TBool active) {
    if (active) {
        Values p{}; for (int i = 0; i < NumParams; ++i) p[i] = values[i].load(std::memory_order_relaxed);
        engine.reset(processSetup.sampleRate, p);
    }
    return AudioEffect::setActive(active);
}
tresult PLUGIN_API Processor::setState(IBStream* stream) {
    Values p{}; if (!readState(stream, p)) return kResultFalse;
    for (int i = 0; i < NumParams; ++i) values[i].store(p[i], std::memory_order_relaxed);
    return kResultOk;
}
tresult PLUGIN_API Processor::getState(IBStream* stream) {
    if (!stream) return kResultFalse;
    IBStreamer s(stream, kLittleEndian);
    if (!s.writeInt32(stateMagic) || !s.writeInt32(1)) return kResultFalse;
    for (auto& value : values) if (!s.writeDouble(value.load(std::memory_order_relaxed))) return kResultFalse;
    return kResultOk;
}
tresult PLUGIN_API Processor::process(ProcessData& data) {
    if (data.numSamples < 0 || (data.symbolicSampleSize != kSample32 && data.symbolicSampleSize != kSample64)) return kResultFalse;
    Values p{}; for (int i = 0; i < NumParams; ++i) p[i] = values[i].load(std::memory_order_relaxed);
    constexpr int maxQueues = 64;
    std::array<int32, maxQueues> point{};
    const int queues = data.inputParameterChanges ? std::min(maxQueues, data.inputParameterChanges->getParameterCount()) : 0;
    auto apply = [&](int sample) {
        for (int q = 0; q < queues; ++q) {
            auto* queue = data.inputParameterChanges->getParameterData(q);
            if (!queue || queue->getParameterId() >= NumParams) continue;
            int32 offset = 0; ParamValue value = 0;
            while (point[q] < queue->getPointCount() && queue->getPoint(point[q], offset, value) == kResultTrue) {
                if (data.numSamples != 0 && offset > sample) break;
                p[queue->getParameterId()] = clean(value);
                values[queue->getParameterId()].store(clean(value), std::memory_order_relaxed);
                ++point[q];
            }
        }
    };
    if (!data.numSamples) { apply(0); return kResultOk; }
    if (data.numOutputs != 1 || !data.outputs || data.outputs[0].numChannels < 1 || data.outputs[0].numChannels > 2)
        return kResultOk;
    auto& out = data.outputs[0];
    auto* in = data.numInputs > 0 && data.inputs ? &data.inputs[0] : nullptr;
    double bpm = 120;
    if (data.processContext && (data.processContext->state & ProcessContext::kTempoValid) && std::isfinite(data.processContext->tempo))
        bpm = data.processContext->tempo;
    out.silenceFlags = 0;
    for (int32 i = 0; i < data.numSamples; ++i) {
        apply(i);
        std::array<double, 2> input{};
        for (int c = 0; c < 2; ++c) {
            int channel = in && in->numChannels > 0 ? std::min(c, in->numChannels - 1) : 0;
            if (in && in->numChannels > 0 && !(in->silenceFlags & (uint64(1) << channel))) {
                if (data.symbolicSampleSize == kSample32 && in->channelBuffers32 && in->channelBuffers32[channel]) input[c] = in->channelBuffers32[channel][i];
                if (data.symbolicSampleSize == kSample64 && in->channelBuffers64 && in->channelBuffers64[channel]) input[c] = in->channelBuffers64[channel][i];
            }
        }
        auto y = engine.process(input[0], input[1], p, bpm);
        for (int c = 0; c < out.numChannels; ++c) {
            if (data.symbolicSampleSize == kSample32 && out.channelBuffers32 && out.channelBuffers32[c]) out.channelBuffers32[c][i] = static_cast<float>(y[c]);
            if (data.symbolicSampleSize == kSample64 && out.channelBuffers64 && out.channelBuffers64[c]) out.channelBuffers64[c][i] = y[c];
        }
    }
    return kResultOk;
}
tresult PLUGIN_API Controller::initialize(FUnknown* host) {
    auto result = EditControllerEx1::initialize(host);
    if (result != kResultOk) return result;
    addUnit(new Unit(STR16("Atomic Cluster"), kRootUnitId, kNoParentUnitId));
    const TChar* titles[]{STR16("Volume"), STR16("Blend"), STR16("Speed"), STR16("Atoms"), STR16("Smooth"), STR16("Flat dynamics"), STR16("Bypass"), STR16("Host sync"), STR16("Beat division"), STR16("Tap tempo enabled")};
    auto initial = defaults();
    for (int i = 0; i < NumParams; ++i) {
        int flags = ParameterInfo::kCanAutomate;
        if (i == Bypass) flags |= ParameterInfo::kIsBypass;
        const int steps = (i >= Smooth && i <= Sync) || i == TapEnabled ? 1 : i == Division ? 4 : 0;
        parameters.addParameter(titles[i], nullptr, steps, initial[i], flags, i);
    }
    return kResultOk;
}
tresult PLUGIN_API Controller::setComponentState(IBStream* stream) {
    Values p{}; if (!readState(stream, p)) return kResultFalse;
    for (int i = 0; i < NumParams; ++i) setParamNormalized(i, p[i]);
    return kResultOk;
}
IPlugView* PLUGIN_API Controller::createView(FIDString type) {
    return type && std::strcmp(type, ViewType::kEditor) == 0 ? createEditor(this) : nullptr;
}
tresult PLUGIN_API Controller::getParamStringByValue(ParamID id, ParamValue v, String128 text) {
    char buffer[128]{}; v = clean(v);
    switch (id) {
    case Volume: std::snprintf(buffer, sizeof(buffer), "%.1f dB", 20 * std::log10(std::max(1e-6, v * 2))); break;
    case Blend: std::snprintf(buffer, sizeof(buffer), "%.0f %% wet", v * 100); break;
    case Speed: std::snprintf(buffer, sizeof(buffer), "%.4f ms", interval(v) * 1000); break;
    case Atoms: std::snprintf(buffer, sizeof(buffer), "%d", atomCount(v)); break;
    case Smooth: std::snprintf(buffer, sizeof(buffer), "%s", v >= .5 ? "Smooth" : "Sharp"); break;
    case Flat: std::snprintf(buffer, sizeof(buffer), "%s", v >= .5 ? "Flat" : "Full"); break;
    case Division: { const char* labels[]{"1/16", "1/8", "1/4", "1/2", "1 bar"}; std::snprintf(buffer, sizeof(buffer), "%s", labels[int(std::round(v * 4))]); break; }
    default: return EditControllerEx1::getParamStringByValue(id, v, text);
    }
    UString(text, 128).fromAscii(buffer);
    return kResultTrue;
}
tresult PLUGIN_API Controller::getParamValueByString(ParamID id, TChar* input, ParamValue& v) {
    if (!input || id >= NumParams) return kResultFalse;
    char text[128]{};
    for (int i = 0; i < 127 && input[i]; ++i) text[i] = char(input[i]);
    const std::string label(text);
    if (id == Division) {
        const char* names[]{"1/16", "1/8", "1/4", "1/2", "1 bar"};
        for (int i = 0; i < 5; ++i) if (label == names[i]) { v = i / 4.; return kResultTrue; }
        return kResultFalse;
    }
    if ((id >= Smooth && id <= Sync) || id == TapEnabled) {
        if (label == "On" || label == "on" || label == "1" || label == "Smooth" || label == "Flat") v = 1;
        else if (label == "Off" || label == "off" || label == "0" || label == "Sharp" || label == "Full") v = 0;
        else return kResultFalse;
        return kResultTrue;
    }
    char* end = nullptr; const double number = std::strtod(text, &end);
    if (end == text || !std::isfinite(number)) return kResultFalse;
    switch (id) {
    case Volume: v = clean(std::pow(10., number / 20.) / 2.); break;
    case Blend: v = clean(number / 100.); break;
    case Speed: if (number <= 0) return kResultFalse; v = clean(std::log(number / 2000.) / std::log(1. / 750.)); break;
    case Atoms: if (number < 1 || number > 1023) return kResultFalse; v = std::log(std::round(number)) / std::log(1023.); break;
    default: return kResultFalse;
    }
    return kResultTrue;
}
}
using namespace Steinberg;
using namespace Steinberg::Vst;
bool InitModule() { return true; }
bool DeinitModule() { return true; }
BEGIN_FACTORY_DEF("Jerzy Audio", "https://github.com/jurekbak71-dev/JerzyAtomicClusterVST3", "")
DEF_CLASS2(INLINE_UID_FROM_FUID(atomiccluster::processorId), PClassInfo::kManyInstances, kVstAudioEffectClass,
    "Jerzy Atomic Cluster", Vst::kDistributable, "Fx|Filter", ATOMIC_VERSION, kVstVersionString, atomiccluster::Processor::create)
DEF_CLASS2(INLINE_UID_FROM_FUID(atomiccluster::controllerId), PClassInfo::kManyInstances, kVstComponentControllerClass,
    "Jerzy Atomic Cluster Controller", 0, "", ATOMIC_VERSION, kVstVersionString, atomiccluster::Controller::create)
END_FACTORY
