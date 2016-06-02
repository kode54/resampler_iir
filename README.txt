Simple resampler

The C++14 version, and the Blam Synthesis C version, both use linear
interpolation, with a three stage IIR filter either on the input or
output stage of the resampler.

The Sinc resampler seems to be faster than the C version, though.
