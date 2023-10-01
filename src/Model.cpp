#include "Model.hpp"

void MyProcessor::operator()(int N)
{
  for(int i = 0; i < this->inputs.audio.channels(); i++)
  {
    auto* in = this->inputs.audio[i];
    auto* out = this->outputs.audio[i];
    for (int j = 0; j < N; j++)
    {
      out[j] = in[j] * inputs.gain;
    }
  }
}
