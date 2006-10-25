
#include <iostream>

#include <math.h>

#include <map>

#include "condition.h"
#include "fetch.h"
#include "Histogram.h"

#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real.hpp>

using namespace std;
using namespace boost;

//#define DBG if (1) 
#define DBG if (0) 


bool Sequence::extend(int len) {
  if (future.size()>=len) {
    return true;
  }
  future.clear();
  if (past.size()>=2) {
    double v1 = past[past.size()-1];
    double dv1 = v1-past[past.size()-2];
    for (int i=0; i<len; i++) {
      double v0 = 0;
      double dv0 = 1e6;
      int idx = 0;
      for (int k=1; k<past.size()-1; k++) {
	double v2 = past[k];
	double dv2 = v2-past[k-1];
	if (fabs(v2-v1)<fabs(v0-v1)
	    && fabs(dv2-dv1)<fabs(dv0-dv1)) {
	  v0 = v2;
	  dv0 = dv2;
	  idx = k;
	}
      }
      idx++;
      v0 = past[idx];
      dv0 = v0-past[idx-1];
      future.push_back(v0);
      v1 = v0;
      dv1 = dv0;
      ///cerr << v1 << " " << dv1 << endl;
    }
  } else {
    for (int i=0; i<len; i++) {
      if (past.size()>=1) {
	future.push_back(past[past.size()-1]);
      } else {
	future.push_back(0);
      }
    }
  }
  return future.size()>=len;
}



bool SymbolicSequence::extend(int len) {
  if (future.size()>=len) {
    return true;
  }

  //cerr << "symbolic extension of degree " << len << endl;
  future = extendSequence(past,len);
  return future.size()>=len;
}


void PiecewiseSequence::apply(const IUnit& unit) {
  DBG cerr << "*********************************************************" << 
    endl;
  DBG cerr << "*** for " << unit.toString().substr(0,20) << " ..." << endl;
  DBG cerr << endl;

  const IUnit& past = unit; //rename

  // check the places of decimal, for cosmetic reasons
  scale = 1;
  places = 1;
  for (int i=1; i<past.size(); i++) {
    double v = past[i];
    for (int k=places; k<=6; k++) {
      if (fabs(v-(long int)(v*scale))<0.0001) {
	break;
      }
      scale *= 10;
      places++;
    }
  }  

  // ok, what kind of numbers do we have?
  bool more = true;
  bool useTh = false;
  double th = 0;
  Histogram deltas;
  Histogram mags;
  while (more) {
    int adds = 0;
    deltas.clear();
    mags.clear();
    more = false;
    int len = 0;
    for (int i=1; i<past.size(); i++) {
      double v = past[i];
      double pv = past[i-1];
      //DBG cerr << past[i] << endl;
      double dv = fabs(v-pv);
      bool add = true;
      if (useTh) {
	add = (dv<=th)&&(dv>=0.000001);
      }
      if (add) {
	adds++;
	deltas.add(dv);
      }
      mags.add(v);
    }
    DBG cerr << "Gap width is " << deltas.getGapWidth() << endl;
    DBG cerr << "adds: " << adds << endl;
    double minFraction = 50;
    minFraction -= past.size(); // scaled for percentage
    if (minFraction<5) {
      minFraction = 5;
    }
    DBG cerr << "Min width is " << minFraction << endl;

    int index = deltas.getGapCenterIndex();
    double v = deltas.getValue(index);
    DBG cerr << "Gap center is " << index << " (" << v << ")" << endl;
    if (deltas.getGapWidth()>minFraction) {
      th = v;
      useTh = true;
      more = true;
    }
  }

  if (unit.size()<6) {
    DBG cerr << "*** Too short, using fixed threshold of 0.5" << endl;
    useTh = true;
    th = 0.5;
  } 



  if (useTh) {
    DBG cerr << "*** Looks roughly piece-wise continuous" << endl;
    DBG cerr << "*** Gap threshold " << th << endl;
  } else {
    DBG cerr << "*** Does not look piece-wise continuous" << endl;
    double v = deltas.getValue(deltas.size()/2);
    th = v;
    DBG cerr << "*** Arbitrary gap threshold " << th << endl;
  }

  bool lenBreak = false;
  int lenBox = 5;;
  if (unit.size()>=6) {
    double globalDev = deltas.getDeviation();
    if (globalDev*3>th) {
      cerr << "strange conditioning on signal" << endl;
      lenBreak = true;
    }
  }

  // okay, suppose we are piece-wise continuous, with a 
  // simple threshold th
  // now let's look at the groups formed relative to th
  // then decide on their identity

  vector<Histogram> hists;
  Histogram hist;
  vector<int> segment(past.size());
  segment[0] = 0;
  int curr = -1;
  for (int i=0; i<past.size(); i++) {
    // note: we skip first number
    double v = past[i];
    double pv = v;
    if (i>0) {
      pv = past[i-1];
    }
    double dv = fabs(v-pv);

    bool breakNow = false;
    int next = mags.getIndex(v)/lenBox;
    if (lenBreak) {
      if (i!=0 && curr!=next) {
	breakNow = true;
      }
      curr = next;
    } else {
      if (dv>=th) {
	breakNow = true;
      }
    }
    if (breakNow) {
      if (hist.size()>0) {
	hists.push_back(hist);
	hist.clear();
      }
      //DBG cerr << endl;
      if (i>0) {
	segment[i] = segment[i-1]+1;
      }
    } else {
      if (i>0) {
	segment[i] = segment[i-1];
      }
    }
    //DBG cerr << v << " ";
    hist.add(v);
  }
  //DBG cerr << endl;
  if (hist.size()>0) {
    hists.push_back(hist);
    hist.clear();
  }
  for (int i=0; i<hists.size(); i++) {
    Histogram& h = hists[i];
    DBG cerr << "Group mean " << h.getMean() << " std " << h.getDeviation() << 
      endl;
  }
  map<int, int> clusterCode;
  for (int i=0; i<hists.size(); i++) {
    clusterCode[i] = i;
  }
  for (int i=0; i<hists.size(); i++) {
    Histogram& h1 = hists[i];
    double m1 = h1.getMean();
    double s1 = h1.getDeviation();
    if (s1<0.5) {
      if (h1.getLength()<2) {
	s1 = 0.45;
      }
    }
    for (int j=i+1; j<hists.size(); j++) {
      Histogram& h2 = hists[j];
      double m2 = h2.getMean();
      double s2 = h2.getDeviation();
      double diff = fabs(m1-m2);
      if (s2<0.5) {
	if (h1.getLength()<2) {
	  s2 = 0.45;
	}
      }
      double scale = 2;
      if (lenBreak) {
	if (mags.getIndex(m1)/lenBox==mags.getIndex(m2)/lenBox) {
	  DBG cerr << "Link " << i << " and " << j << endl;
	  if (clusterCode[j]>i) {
	    clusterCode[j] = i;
	  }
	}
      } else {
	if (diff<(scale*s1+0.00001)||diff<(scale*s2+0.00001)) {
	  DBG cerr << "Link " << i << " and " << j << endl;
	  if (clusterCode[j]>i) {
	    clusterCode[j] = i;
	  }
	}
      }
    }
  } 
  map<int, int> label;
  map<int, int> revLabel;
  int ct = 0;
  for (int i=0; i<hists.size(); i++) {
    int localCode = clusterCode[i];
    if (label.find(localCode)==label.end()) {
      label[localCode] = ct;
      revLabel[ct] = localCode;
      ct++;
    }
  }  
  int groupCount = ct;
  DBG cerr << "sequence: ";
  sym.clear();
  for (int i=0; i<hists.size(); i++) {
    sym.add(label[clusterCode[i]]);
    DBG cerr << label[clusterCode[i]] << " ";
  }
  DBG cerr << endl;
  DBG cerr << "Unit is: " << toString() << endl;

  cerr << "Symbolic form of " << unit.toString() << " is " << 
    sym.toString() << endl;

  vector<Histogram> globalHists(groupCount);
  vector<Histogram> lengthHists(groupCount);
  vector<Sequence> lengthSeqs(groupCount);
  DBG cerr << "group count is " << groupCount << endl;
  for (int i=0; i<past.size(); i++) {
    int idx = label[clusterCode[segment[i]]];
    DBG cerr << "  offset " << i << " segment " << segment[i] << " code "
	     << idx 
	     << endl;
    Histogram& h = globalHists[idx];
    h.add(past[i]);
  }
  for (int i=0; i<hists.size(); i++) {
    int len = hists[i].getLength();
    DBG cerr << "add len " << len << endl;
    lengthHists[label[clusterCode[i]]].add(len);
    lengthSeqs[label[clusterCode[i]]].add(len);
  }

  parts = vector<Piece>(groupCount);
  Histogram global;
  for (int i=0; i<groupCount; i++) {
    Piece& p = parts[i];
    p.mean = globalHists[i].getMean();
    p.dev = globalHists[i].getDeviation();
    p.len = lengthHists[i].getMean();
    //if (lengthSeqs[i].size()==0) {
    //lengthSeqs[i].add(p.len);
    //}
    p.lengthUnit = specialize(lengthSeqs[i]);
    global.add(p.len);
    DBG cerr << "Piece " << i << " mean " << p.mean << endl;
  }
  meanLength = global.getMean();

  DBG cerr << endl;
  DBG cerr << "*** for " << unit.toString().substr(0,20) << " ..." << endl;
  DBG cerr << "*********************************************************" << 
    endl;

}


bool PiecewiseSequence::extendParts(int len, int pad) {
  bool ok = false;
  int target = 1+int(1.4*len/meanLength)+pad;
  ok = sym.extend(target);
  if (!ok) return false;
  for (int i=0; i<parts.size(); i++) {
    ok = parts[i].lengthUnit.extend(target);
    if (!ok) return false;
  }
  return ok;
}



bool PiecewiseSequence::extend(int len) {
  if (future.size()>=len) {
    return true;
  }
  if (!extendParts(len,0)) {
    return false;
  }
  int pad = 0;
  mt19937 rng;
  rng.seed((unsigned int) time(0));
  while (future.size()<len) {
    future.clear();
    map<int,int> individualCounts;
    for (int i=0; i<sym.futureSize(); i++) {
      int code = (int)(sym.predict(i)+0.5);
      if (code<0) {
	 boost::uniform_real<> uni_dist(0,1);
	 boost::variate_generator<mt19937&, boost::uniform_real<> >
	   uni(rng, uni_dist);
	 code = int(uni()*parts.size());
	 cerr << "(weak random unit replacement)" << endl;
      }
      Piece& p = parts[code];
      normal_distribution<double> dist(p.mean,p.dev);
      variate_generator<mt19937&, normal_distribution<double> > 
	normal_sampler(rng, dist);
      double len = p.len;
      if (individualCounts.find(code)==individualCounts.end()) {
	individualCounts[code] = 0;
      }
      len = p.lengthUnit.predict(individualCounts[code]);
      individualCounts[code]++;
      for (int j=0; j<int(len+0.5); j++) {
	double v = normal_sampler();
	v = (double)((long int)(v*scale+0.5))/scale;
	future.push_back(v);
      }
    }
    if (future.size()<len) {
      pad += 4;
      DBG cerr << "future size is " << future.size() << endl;
      DBG cerr << "extending sym to " << len+pad << endl;
      if (!extendParts(len,pad)) { 
	return false;
      }
    }
  }
  return true;
}


double ConstantSequence::predict(int index) {
  extend(index+1);
  mt19937 rng;
  double mean = h.getMean();
  double dev = h.getDeviation();
  if (dev<0.0001) {
    return mean;
  }
  normal_distribution<double> dist(mean,dev);
  variate_generator<mt19937&, normal_distribution<double> > 
    normal_sampler(rng, dist);
  double v = normal_sampler();
  return v;
}




IUnit *IUnit::specialize(const IUnit& source) {
  // I should create a unit that is appropriate for my source

  bool varies = false;
  double maxd = 0;
  Histogram h;
  for (int i=1; i<source.size(); i++) {
    if (source[i]!=source[0]) {
      varies = true;
    }
    double dx = source[i]-source[i-1];
    if (dx>maxd) {
      maxd = dx;
    }
    h.add(source[i]);
  }

  DBG cerr << "Max deviation is " << maxd << " versus " 
	   << h.getDeviation() << endl;

  if (maxd<h.getDeviation() && source.size()>=20) {
    Sequence *seq = new Sequence();
    assert(seq!=NULL);
    seq->copy(source);
    return seq;
  }

  if (source.size()>=2 && varies) {
    PiecewiseSequence *pw = new PiecewiseSequence();
    assert(pw!=NULL);
    pw->apply(source);
    return pw;
  }
  IUnit *c = new ConstantSequence();
  assert(c!=NULL);
  c->copy(source);
  return c;
}
