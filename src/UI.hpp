#pragma once
#include "Processor.hpp"

#include <halp/layout.hpp>
#include <halp/meta.hpp>

struct MyProcessor::ui
{
  // If your compiler is recent enough:
  // using enum halp::colors;
  // using enum halp::layouts;
  halp_meta(layout, halp::layouts::vbox)
  halp_meta(background, halp::colors::mid)

  halp::label header{"Hello !"};

  struct
  {
    halp_meta(layout, halp::layouts::hbox)
    halp_meta(background, halp::colors::dark)
    //halp::item<&ins::gain> widget;
  } widgets;
};
