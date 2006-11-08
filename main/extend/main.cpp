
#include <stdio.h>
#include <iostream>

#include "fetch.h"
#include "condition.h"

#include "yarp.h"

using namespace std;



static vector<double> getSeq() {
  vector<double> result;
  while (!cin.eof()) {
    double v = 0;
    cin >> v;
    if (!cin.eof()) {
      result.push_back(v);
    }
  }
  return result;
}



int main(int argc, char *argv[]) {
  if (argc>1) {
    std::string seq = argv[1];
    if (seq=="net") {
      return netmain(argc,argv);
    }

    std::string pattern = sequenceToPattern(seq);
    printf("pattern is %s\n", pattern.c_str());
    if (argc>2) {
      int ct = atoi(argv[2]);
      std::string seqExt = extendSequence(seq,pattern,ct);
      printf("extended sequence by %d from %s: %s\n", 
	     ct, seq.c_str(), seqExt.c_str());
	     
    }
  } else {
#if 1
    double data[] = {
      0, 1, 0, 3, 2, 3, 
      20, 21, 19, 
      0, 1, 0, 1, 5, 2, 4,
      22, 17, 18, 24,
      -999
    };
#else
    double data[] = {
      0, 1, 2, 3, 4, 5, 6, 6, 8, 9, 10, -999
    };
#endif
    double *at = data;
    Sequence seq;
    //while (*at>=-0.5) {
    //seq.add(*at);
    //at++;
    //}

    vector<double> lst = getSeq();
    for (int i=0; i<lst.size(); i++) {
      seq.add(lst[i]);
    }

    Unit unit(seq.specialize(seq));

    //cout << "piecewise for " << seq.toString() << endl;
    //PiecewiseSequence ps;
    //ps.apply(seq);
    //cout << "Unit is: " << ps.toString() << endl;
    Sequence fut;
    fut.takeFuture(unit,100);
    cout << fut.toString() << endl;

  }
  return 0;
}

