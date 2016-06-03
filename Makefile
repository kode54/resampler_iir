CFLAGS = -O0 -g
CXXFLAGS = -O0 -g -I. -std=c++14

OBJS = resampler.o limiter.o

K54_OBJS = resampler_k54.o resampler_c.o limiter.o

K54_SINC_OBJS = resampler_k54_sinc.o resampler_c.o limiter.o

SOX_OBJS = resampler_sox.o limiter.o

all: resampler resampler_k54 resampler_k54_sinc resampler_sox

resampler : $(OBJS)
	$(CXX) -o $@ $^

resampler_k54 : $(K54_OBJS)
	$(CXX) -o $@ $^

resampler_k54_sinc : $(K54_SINC_OBJS)
	$(CXX) -o $@ $^

resampler_sox : $(SOX_OBJS)
	$(CXX) -o $@ $^ -lsoxr

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $*.cpp

resampler_c.o : k54/resampler.c
	$(CC) $(CFLAGS) -D__NALL__ -c -o $@ $^

resampler_k54.o : resampler.cpp
	$(CXX) $(CXXFLAGS) -D__K54__ -c -o $@ $^

resampler_k54_sinc.o : resampler.cpp
	$(CXX) $(CXXFLAGS) -D__K54__ -D__SINC__ -c -o $@ $^

resampler_sox.o : resampler.cpp
	$(CXX) $(CXXFLAGS) -D__SOX__ -c -o $@ $^

clean:
	rm -f $(OBJS) $(K54_OBJS) $(K54_SINC_OBJS) $(SOX_OBJS) resampler resampler_k54 resampler_k54_sinc resampler_sox > /dev/null
