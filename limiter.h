#ifndef _limiter_h_
#define _limiter_h_

namespace monkee_limiter
{

enum
{
	bufPow=8,
	bufLength=1<<bufPow,
	bufMax =	bufLength - 1,
};

class limiter
{
	int bufPos;

	float backbuffer[bufLength];

	float buffer[bufLength*2];
	
	float gain,minGainLP;

	bool active;

public:
	limiter();

	float process_sample(float in);
};

}
    
#endif
