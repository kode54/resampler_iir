#include <nall/nall.hpp>

#ifndef __K54__
#include <nall/dsp/iir/biquad.hpp>
#include <nall/dsp/resampler/linear.hpp>
#endif

#include "limiter.h"

#include "k54/resampler.h"

#ifdef __SINC__
#define USE_QUALITY RESAMPLER_QUALITY_SINC
#else
#define USE_QUALITY RESAMPLER_QUALITY_BLAM
#endif

using namespace nall;

#ifndef __K54__
struct Stream {
  const uint order = 6;  //Nth-order filter (must be an even number)
  struct Channel {
    vector<DSP::IIR::Biquad> iir;
    DSP::Resampler::Linear resampler;
  };
  vector<Channel> channels;
  bool outputstage;

  inline auto pending() const -> bool {
    return channels && channels[0].resampler.pending();
  }

  inline auto read(double * samples) -> uint {
    for(auto c : range(channels)) {
      samples[c] = channels[c].resampler.read();
      if (outputstage)
        for(auto& iir : channels[c].iir) samples[c] = iir.process(samples[c]);
    }
    return channels.size();
  }

  inline auto write(const double * samples) -> void {
    for(auto c : range(channels)) {
      double sample = samples[c] + 1e-25;  //constant offset used to suppress denormals
      if (!outputstage)
        for(auto& iir : channels[c].iir) sample = iir.process(sample);
      channels[c].resampler.write(sample);
    } 
  }

  auto reset(uint channels_, double inputFrequency, double outputFrequency) -> void {
    size_t oldsize = channels.size();

    if (oldsize != channels_) {
      channels.reset();
      channels.resize(channels_);
    }

    double ratio_ = outputFrequency / inputFrequency;

    outputstage = (ratio_ >= 1.0);

    if (!outputstage) {
      ratio_ = (outputFrequency * 0.45) / inputFrequency;
    }
    else {
      ratio_ = (inputFrequency * 0.45) / outputFrequency;
    }
    ratio_ = min(ratio_, 0.45);

    for(auto& channel : channels) {
      channel.iir.resize(order / 2);
      for(auto phase : range(order / 2)) {
        double q = DSP::IIR::Biquad::butterworth(order, phase);
        channel.iir[phase].reset(DSP::IIR::Biquad::Type::LowPass, ratio_, q);
      }

      channel.resampler.reset(inputFrequency, outputFrequency);
    }
  }
};
#endif

#include <nall/main.hpp>
auto nall::main(lstring args) -> void {
  nall::file rd, wr;
  double inFreq, outFreq;
  bool bench = false;

  if (args.size() == 2 && args[1] == "bench")
    bench = true;

  if (args.size() != 5 && !bench) {
    print("Usage:\tresampler <input.raw> <output.raw> <source rate> <target rate>\n   or\n\tresampler bench\n\n");
    return;
  }

  if (bench) {
    inFreq = 384000 * 8;
    outFreq = 44100;
  }

#ifdef __K54__
  resampler_init();

  void * resampler = resampler_create();

  resampler_set_quality(resampler, USE_QUALITY);

  resampler_set_rate(resampler, inFreq / outFreq);
#else
  Stream dsp;

  dsp.reset(1, inFreq, outFreq);
#endif

  while (bench) {
    uint32 count = inFreq * 60 * 2; // two minutes

    while (count) {
      int16 sample16 = (int16) random();
      double sample = (sample16 * 1.0 / 32768.0);
      --count;
#ifdef __K54__
      resampler_write_sample_float(resampler, sample);
      while (resampler_get_sample_count(resampler))
        resampler_remove_sample(resampler, 0);
#else
      dsp.write(&sample);
      while (dsp.pending())
        dsp.read(&sample);
#endif
    }

    if (inFreq == 11025)
      return;
    else {
      inFreq = 11025;
      outFreq = 44100;
#ifdef __K54__
      resampler_set_rate(resampler, inFreq / outFreq);
#else
      dsp.reset(1, inFreq, outFreq);
#endif
    }
  }

  inFreq = nall::real( args[3] );
  outFreq = nall::real( args[4] );

  if (inFreq <= 0.0 || inFreq > 384000.0) {
    print("Invalid source rate: ", inFreq, "\n\n");
    return;
  }

  if (outFreq <= 0.0 || outFreq > 384000.0) {
    print("Invalid target rate: ", outFreq, "\n\n");
    return;
  }

  monkee_limiter::limiter lim;

  rd.open( args[1], file::mode::read );
  wr.open( args[2], file::mode::write );

  uint32 sample32 = rd.readl(4);
  float sample;

  while (sample32 != 0xFFFFFFFF) {
    *(uint32 *)(&sample) = sample32;
    double sampled = sample;
#ifdef __K54__
   resampler_write_sample_float(resampler, sampled);
   while (resampler_get_sample_count(resampler)) {
     sampled = resampler_get_sample_float(resampler);
     resampler_remove_sample(resampler, 0);
     sample = sampled;
     uint32 sampleout32;
     *(float *)(&sampleout32) = sample;
     wr.writel(sampleout32, 4);
   }
#else
   dsp.write(&sampled);
    while (dsp.pending()) {
      dsp.read(&sampled);
      sample = sampled;
      sample = lim.process_sample(sample) * 0.999;
      uint32 sampleout32;
      *(float *)(&sampleout32) = sample;
      wr.writel(sampleout32, 4);
    }
#endif
    sample32 = rd.readl(4);
  }

#ifdef __K54__
  resampler_delete(resampler);
#endif
}

