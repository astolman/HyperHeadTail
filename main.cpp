//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
//main file for doing shht
#include "head_map.hpp"
#include "tail_map.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
#include <fstream>
#include "true_ccdh_calculator.hpp"
#include <stdlib.h>

using namespace std;
namespace po = boost::program_options;

   

function<long double (uint32_t)> read_table(string file) {
   //expects to find a text file with a window size and 
   //p-value on each line seperated by white space
   ifstream input;
   input.open(file);
   if (! input.is_open()) {
      cerr << "Error: could not open " << file << ".\n";
      exit(1);
   }
   vector<uint32_t> keys;
   unordered_map<uint32_t, long double> table;
   uint32_t x;
   long double y;
   while (input >> x >> y) {
      table[x] = y;
      keys.push_back(x);
   }
   return std::function<long double (uint32_t)>([keys, table](uint32_t window)-> long double {
         auto itor = table.find(window);
         if (itor != table.end())
            return itor->second;
         int interval_len = keys.size();
         int interval_start = 0;
         int interval_end = keys.size();
         if (keys.back() < window) 
            return static_cast<long double>(0);
         while (interval_len != 1) {
            int pivot = (interval_end - interval_start)/2 + interval_start;
            if (keys[pivot] < window) {
               interval_start = pivot;
               interval_len = interval_end - interval_start;
            } else {
               interval_end = pivot;
               interval_len = interval_end-interval_start;
            }
         }
         return table.at(keys[interval_start]); 
      });
}
   
//helper function to calculate dthr
uint64_t find_dthr (vector<u64pair> gh, long double ph) {
   uint64_t sum = 0;
   uint64_t target = 50 / ph;
   uint64_t d = 0;
   for ( auto it = gh.rbegin(); it != gh.rend(); it++) {
      d = it->first;
      sum += it->second;
      if (sum >= target) break;
   }
   return d;
}

//helper function to actually open a textfile and print the output to it
void make_output (vector<u64pair> ccdh, uint32_t window, string output, uint64_t dthr) {
   ofstream out;
   out.open(output, ios_base::app);
   string xs("[");
   string ys("[");
   auto it = ccdh.begin();
   xs += to_string(it->first);
   ys += to_string(it->second);
   if ( it != ccdh.end() ) 
      for (++it; it != ccdh.end(); it++) {
         xs += ", " + to_string(it->first);
         ys += ", " + to_string(it->second);
      }
   xs += "]";
   ys += "]";
   out << "   Estimate, window = " << window <<"\n";
   out << "      xs = "<<xs<<"\n";
   out << "      ys = "<<ys<<"\n";
   out << "      dthr = " <<dthr << "\n";
   out.close();
}

pair<vector<u64pair>, uint64_t> calculate_ccdh(unique_ptr<head_map> &Sh, tail_map* St, uint32_t timestamp, uint32_t window) {
   vector<u64pair> tail = St->estimate(timestamp, window);
   vector<u64pair> head = Sh->estimate(timestamp, window);
   //dthr = find_dthr (head, Sh->get_ph());
   uint64_t dthr = find_dthr(head, Sh->get_ph(window));
   vector<u64pair> ccdh;
   ccdh.reserve(tail.size() + head.size());
   uint64_t sum = 0;
   for (auto it = tail.rbegin(); it != tail.rend() && it->first > dthr; ++it) {
      sum += it->second;
      ccdh.push_back(u64pair(it->first, sum));
   }
   for (auto it = head.rbegin(); it != head.rend(); it++) {
      if (it->first > dthr) continue;
      sum += it->second;
      ccdh.push_back(u64pair(it->first, sum));
   }
   return pair<vector<u64pair>, uint64_t> (ccdh, dthr);
}

void make_true_output (vector<u64pair> ccdh, uint32_t window, string output) {
   ofstream out;
   out.open(output, ios_base::app);
   string xs("[");
   string ys("[");
   auto it = ccdh.begin();
   xs += to_string(it->first);
   ys += to_string(it->second);
   for (++it; it != ccdh.end(); it++) {
      xs += ", " + to_string(it->first);
      ys += ", " + to_string(it->second);
   }
   xs += "]";
   ys += "]";
   out << "   True distro, Window = " << window <<"\n";
   out << "      xs = "<<xs<<"\n";
   out << "      ys = "<<ys<<"\n";
   out.close();
}

//function to read through the input stream and update the data structures
uint32_t stream_edges (unique_ptr<head_map> &Sh, tail_map* St, true_ccdh_calculator* tcc,ifstream* stream, bool get_true, uint32_t pause) {
   int i = 0;
   uint64_t u, v;
   uint32_t timestamp;
   if (get_true) {
      while ( *stream >> u >> v >> timestamp) {
         Sh-> update (u, v, timestamp);
         Sh-> update (v, u, timestamp);
         St-> update (u, v, timestamp);
         St-> update (v, u, timestamp);
         tcc->add_edge (u, v, timestamp);
      }
   } else {
      while ( *stream >> u >> v >> timestamp) {
         Sh-> update (u, v, timestamp);
         Sh-> update (v, u, timestamp);
         St-> update (u, v, timestamp);
         St-> update (v, u, timestamp);
      }
   }
   return timestamp;
}

int main(int ac, const char * av[]) {
   /*~~~~~PARSE COMMAND LINE OPTIONS ~~~~~~~~~~~~*/
   string input;
   string output;
   function<long double(uint32_t)> ph_decay, pt_decay;
   po::options_description desc("Allowed options");
   desc.add_options()
      ("help", "produce help message")
      ("input", po::value<std::string>(), "set source file for graph. If this option is not specified, HHT reads from multigraph.txt. All input files should consist of lines of the form '[u] [v] [t]' and nothing else. Here u and v are nodes in the graph and t is the timestamp. Timestamps appear in increasing order.")
      ("output", po::value<string>(), "set output file. See README for formatting of output file.")
      ("ph", po::value<long double>(), "set ph for constant ph runs of HHT. Will not call the clean_Sh method. If this option is not set and neither is --get-ph-decay, ph will be set to a default value of .1")
      ("pt", po::value<long double>(), "set pt for constant ph runs of HHT. Will not call the clean_St method. If this option is not set and neither is --get-pt-decay, pt will be set to a default value of .1")
      ("precision", po::value<short>(), "set precision of SHLL counter objects")
      ("debug", "debug flag")
      ("epsilon", po::value<double>(), "DEPRECATED")
      ("max-window", po::value<uint32_t>(), "set max time for sliding window, if not set, and using constant ph and pt, will default to a value of 10000")
      ("get-true-distribution", "get the real ccdh for comparison.\nThis should be way less space-efficient.")
      ("pause", po::value<uint32_t>(), "[UNDER DEVELOPMENT] - set timestamp for it to pause and take snapshot of.")
      ("get-size-info", "flag to turn on printing size of nodes to std err.")
      ("get-pt-decay", po::value<std::string>(), "set decay table for pt, argument is a file where each line is of the format '[window] [probability]' where the (window, probability) pairs are in strict increasing order of window and decreasing order of probability. These points define a step function for pt")
      ("get-ph-decay", po::value<std::string>(), "set step function for ph. Argument is a file formatted as with --get-pt-decay")
   ;
   po::variables_map vm;
   po::store(po::parse_command_line(ac, av, desc), vm);
   po::notify(vm);
   if (vm.count("help")) {
      cout << desc << "\n";
      return 1;
   }
   if (vm.count("get-ph-decay")) 
      ph_decay = read_table(vm["get-ph-decay"].as<std::string>());
   if (vm.count("get-pt-decay")) 
      pt_decay = read_table(vm["get-pt-decay"].as<std::string>());
   if (vm.count("input")) {
      input = vm["input"].as<std::string>();
      cout << "Reading from " << vm["input"].as<std::string>() << "...\n";
   } else {
      cout << "Input not set\nReading from multigraph.txt...\n";
      input = "multigraph.txt";
   }
   if (vm.count("output")) {
      output = vm["output"].as<std::string>();
   } else {
      cout << "Output not specified, writing to ./myoutput\n";
      output = "./myoutput";
   }
   long double ph = (vm.count("ph"))? vm["ph"].as<long double>() : 0.1;
   long double pt = (vm.count("pt"))? vm["pt"].as<long double>() : 0.1; 
   uint32_t pause = (vm.count("pause"))? vm["pause"].as<uint32_t>() : 0;
   bool debug = vm.count("debug");
   bool get_true = vm.count("get-true-distribution");
   short m = (vm.count("precision"))? vm["precision"].as<short>() : 8;
   double epsilon = (vm.count("epsilon"))? vm["epsilon"].as<double>() : 0.05;
   uint32_t W = (vm.count("max-window"))? vm["max-window"].as<uint32_t>() : 10000;
   bool size_flag = vm.count("get-size-info");

   ifstream graph;
   graph.open(input);
   if (! graph.is_open()) {
      cerr << "Error: could not open " << input << ".\n";
      return 1;
   }


   //Now we can run the algorithm
   true_ccdh_calculator tcc;
   unique_ptr<head_map> Sh;
   if (vm.count("get-ph-decay"))
      Sh = unique_ptr<head_map>(new fancy_head(ph_decay, W, m));
   else
      Sh = unique_ptr<head_map>(new simple_head(ph, W, m));
   tail_map* St;
   if (vm.count("get-pt-decay"))
      St = new tail_map(pt_decay, W, m);
   else
      St = new tail_map(pt, W, m);

   uint32_t end_time = stream_edges (Sh, St, &tcc, &graph, get_true, pause);
   graph.close();
   
   //print size info if wanted
   if (size_flag) {
      vector<pair<int, int> > tail_sizes = St->get_size();
      vector<pair<int, int> > head_sizes = Sh.get()->get_size();
      cerr << "Head info histogram:\n";
      fprintf(stderr, "num_entries : frequency\n");
      for (auto it = head_sizes.begin(); it != head_sizes.end(); ++ it) 
         fprintf (stderr, "  %d : %d\n", it->first, it->second);
      cerr << "Tail info histogram:\n";
      for (auto it = tail_sizes.begin(); it != tail_sizes.end(); ++it)
         fprintf (stderr, "  %d : %d\n", it->first, it->second);
   }
   ofstream out;
   out.open(output, ios_base::app);
   out << "File : " << input <<", length = " << end_time << ", Head size = " << Sh->size() <<", Tail size = " << St->size() << " ph = " << ph << ", pt = " << pt << "\n";
   out.close();
   uint32_t window = 0;
   cout << "For what window would you like to estimate a ccdh?\n";
   while (cin >> window) {
      pair<vector<u64pair>, uint64_t> estimate = calculate_ccdh(Sh, St, end_time, window);
      make_output(estimate.first, window, output, estimate.second);
      if (get_true) make_true_output (tcc.get_ccdh(end_time, window), window,  output);
      cout << "Done! Enter another window or <EOF> to quit.\n";
   }
   delete St;
}
