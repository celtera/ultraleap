#include "Processor.hpp"

void MyProcessor::operator()(double** ins, double** outs, int N)
{
  for (int i = 0; i < channels(); i++)
  {
    auto* in = ins[i];
    auto* out = outs[i];
    for (int j = 0; j < N; j++)
    {
      out[j] = in[j] * inputs.gain;
    }
  }
}