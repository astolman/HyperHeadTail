# the following line assumes that openssl source is in /opt/local/include
CXX = ~/gcc-4.5.0/bin/g++
CXXFLAGS = -g -std=c++17 -O3 -I /opt/local/include/
COUNT_OBJS = empirical_data.o shll.o utility.o nearest_neighbor.o
OBJS = $(COUNT_OBJS) test.o

libcount.a : $(COUNT_OBJS)
	ar rcs libcount.a $(COUNT_OBJS)
	ranlib libcount.a

test : $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ -lcrypto 

%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $<

clean : 
	rm libcount.a *.o test
