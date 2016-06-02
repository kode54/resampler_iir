#include <math.h>
#include <string.h>

#include "limiter.h"

namespace monkee_limiter
{

static inline float f_max(float v1,float v2)
{	
	return v1>v2 ? v1 : v2;
}

static const float limiter_max = (float)0.9999;

limiter::limiter()
{
	bufPos = 0;
    gain		= 1.0f,
	minGainLP	= 1.0f;
    active = false;
}

float limiter::process_sample(float in)
{
    float max,out;

	float val = (float)fabs(in);
		
    if (val > limiter_max)
    {
        if (!active) memset(buffer,0,sizeof(buffer));
        active = true;
    }

    {
        if (active)
        {
            int offset = bufPos, level = bufPow;
            do
            {
                float * ptr = buffer + bufLength*2 - (2<<level);
				ptr[offset]=val;
                val = f_max(val,ptr[offset^1]);
                offset>>=1;
                level--;
            } while(level>0);
            max = val;
            if (max<=limiter_max) active = false;
        }
			

        backbuffer[bufPos] = in;
        bufPos = (bufPos+1)&bufMax;
        out = backbuffer[bufPos];
    }

    // attenute when getting close to the point of clipping
    float minGain;

    if(active)
        minGain = limiter_max/max;
    else
        minGain = limiter_max;
			
		
    // Low-pass filter of the control signal
    minGainLP = 0.9f*minGainLP + 0.1f * minGain;

    // choose the lesser of these two control signals
    gain = 0.001f + 0.999f*gain;
    if (minGainLP<gain) gain = minGainLP;
		
    if (fabs(out*gain)>limiter_max) gain = (float)(limiter_max/fabs(out));
		
    return out * gain;
}

}
