CXXFLAGS = -g -O3 -std=c++17 -I ./shll/ -I ~/build/boost_1_61_0 
CXX = g++
OBJS = head_map.o tail_map.o true_ccdh_calculator.o main.o
LDFLAGS = ~/build/boost_1_61_0/bin.v2/libs/program_options/build/gcc-7.0.0/release/link-static/threading-multi/libboost_program_options.a 

hht : $(OBJS) shll/libcount.a
	$(CXX) $(CXXFLAGS) $(OBJS) shll/libcount.a -o $@ $(LDFLAGS) -lcrypto

test : $(OBJS) shll/libcount.a
	g++ $(CXXFLAGS) $(OBJS) shll/libcount.a -o $@

shll/libcount.a : shll/shll.h shll/shll.cc
	cd shll; make libcount.a

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $<

clean : 
	rm *.o hht; cd shll; make clean
