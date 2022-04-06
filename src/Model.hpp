#pragma once
#include <halp/controls.hpp>
#include <halp/meta.hpp>

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
  halp_meta(channels, 2)

  struct ins
  { halp::hslider_f32<"Gain", halp::range{.min = 0., .max = 10., .init = 0.5}> gain;
  } inputs;

  struct
  {
  } outputs;

  void operator()(double** ins, double** outs, int N);

  // Defined in UI.hpp
  struct ui;
};