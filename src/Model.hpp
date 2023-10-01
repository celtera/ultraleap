#pragma once
#include <halp/audio.hpp>
#include <halp/controls.hpp>
#include <halp/meta.hpp>

#include <iostream>

class MyProcessor
{
public:
  halp_meta(name, "My Processor")
  halp_meta(c_name, "my_processor")

  // CHANGE THIS !!
  // - On linux: uuidgen | xargs printf | xclip -selection clipboard
  //   will copy one on the clipboard
  // - uuidgen exists on Mac and Windows too
  halp_meta(uuid, "00000000-0000-0000-0000-000000000000")
  // halp_meta(channels, 2)

  struct
  {
    halp::fixed_audio_bus<"Input", float, 1> audio;
    struct : halp::hslider_f32<"Gain", halp::range{.min = 0., .max = 1., .init = 0.5}>
    {
      void update(MyProcessor& m) { std::cerr << "okie " << value << "\n"; }
    } gain;
  } inputs;

  struct
  {
    halp::fixed_audio_bus<"Input", float, 1> audio;
  } outputs;

  void operator()(int N);

  // Defined in UI.hpp
  struct ui;
};
