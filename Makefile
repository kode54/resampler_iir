CFLAGS = -O2
CXXFLAGS = -O2 -I.. -std=c++14

OBJS = resampler.o limiter.o

K54_OBJS = resampler_k54.o resampler_c.o limiter.o

K54_SINC_OBJS = resampler_k54_sinc.o resampler_c.o limiter.o

all: resampler resampler_k54 resampler_k54_sinc

resampler : $(OBJS)
	$(CXX) -o $@ $^

resampler_k54 : $(K54_OBJS)
	$(CXX) -o $@ $^

resampler_k54_sinc : $(K54_SINC_OBJS)
	$(CXX) -o $@ $^

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $*.cpp

resampler_c.o : k54/resampler.c
	$(CC) $(CFLAGS) -c -o $@ $^

resampler_k54.o : resampler.cpp
	$(CXX) $(CXXFLAGS) -D__K54__ -c -o $@ $^

resampler_k54_sinc.o : resampler.cpp
	$(CXX) $(CXXFLAGS) -D__K54__ -D__SINC__ -c -o $@ $^

clean:
	rm -f $(OBJS) $(K54_OBJS) $(K54_SINC_OBJS) resampler > /dev/null
