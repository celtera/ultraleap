#pragma once

#include <avnd/helpers/controls.hpp>
#include <avnd/helpers/meta.hpp>

#include <vector>

class MyProcessor
{
public:
  $(name, "My Processor")
  $(c_name, "my_processor")

  // CHANGE THIS !!
  // - On linux: uuidgen | xargs printf | xclip -selection clipboard
  //   will copy one on the clipboard
  // - uuidgen exists on Mac and Windows too
  $(uuid, "00000000-0000-0000-0000-000000000000")
  $(channels, 2)

  struct
  { avnd::hslider_f32<"Gain", avnd::range{.min = 0., .max = 10., .init = 0.5}> gain;
  } inputs;

  struct
  {
  } outputs;

  void operator()(double** ins, double** outs, int N);
};