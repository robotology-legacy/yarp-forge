
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
    Sequence seq;

    vector<double> lst = getSeq();
    for (int i=0; i<lst.size(); i++) {
      seq.add(lst[i]);
    }

    Unit unit(seq.specialize(seq));

    Sequence fut;
    fut.takeFuture(unit,lst.size()*5);
    cout << fut.toString() << endl;

  }
  return 0;
}

