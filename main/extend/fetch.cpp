#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/random/uniform_real.hpp>
//#include <boost/functional/hash.hpp>
//#include <boost/functional/hash/map.hpp>

#include <map>
#include <algorithm>
#include <boost/random.hpp>
#include <ctime>

#include "fetch.h"

#define DBG if (0) 
//#define DBG if (1) 

using namespace std;
using namespace boost;
//using namespace boost::functional;

class Normalizer {
private:
  int at;
  string all;
  map<char, char> dict;
  map<char, char> rdict;
  string seq;
public:
  Normalizer() {
    at = 0;
    seq = "0123456789abcdefghijklmnopqrstuvwxyz";
  }

  char toChar(int x) {
    if (x<seq.length()) {
      return seq[x];
    }
    return -1;
  }

  void add(std::string s) {
    for (int i=0; i<s.length(); i++) {
      char ch = s[i];
      dict[ch] = seq[at];
      rdict[seq[at]] = ch;
      all = all+ch;
      at++;
    }
  }

  bool check(std::string s) {
    for (int i=0; i<s.length(); i++) {
      if (seq.find(s[i])==seq.npos) {
	return false;
      }
    }
    return true;
  }

  std::string norm(std::string s) {
    bool quoted = false;
    for (int i=0; i<s.length(); i++) {
      char ch = s[i];
      if (quoted) {
	quoted = false;
	break;
      }
      if (ch=='\\') {
	quoted = true;
	break;
      }
      if (seq.find(ch)!=seq.npos) {
	if (dict.find(ch)==dict.end()) {
	  if (at>=seq.length()) {
	    cerr << "Sequence too long!" << endl;
	    exit(1);
	  }
	  DBG cout << "mapping " << ch << " <--> " << seq[at] << " :"
		   << seq.find(ch) << endl;
	  dict[ch] = seq[at];
	  rdict[seq[at]] = ch;
	  all = all+ch;
	  at++;
	}
	s[i] = dict[ch];
      }
    }
    return s;
  }

  std::string denorm(std::string s) { 
    bool quoted = false;
    for (int i=0; i<s.length(); i++) {
      char ch = s[i];
      if (quoted) {
	quoted = false;
	break;
      }
      if (ch=='\\') {
	quoted = true;
	break;
      }
      if (rdict.find(ch)!=rdict.end()) {
	s[i] = rdict[ch];
      }
    }
    return s;
  }

  std::string getInput() {
    return all;
  }
};


static void checkSequence(string seq) {
  Normalizer n;
  bool ok = n.check(seq);
  if (!ok) {
    cerr << "Sequence can contain only numbers 0-9 and lowercase a-z" << endl;
    exit(1);
  }
}


std::string localizePattern(std::string pattern) {
  DBG cout << "localizing pattern " << pattern << endl;

  try {

    boost::regex_constants::syntax_option_type flags = 
      boost::regex_constants::perl;
    boost::regex re;
    boost::smatch what;
    
    
    re.assign("\\[\\]", flags);
    pattern = regex_replace(pattern,re,"");
    re.assign("\\(", flags);
    pattern = regex_replace(pattern,re,"\\(\\?\\:");
    re.assign("\\[", flags);
    pattern = regex_replace(pattern,re,"\\(\\[");
    re.assign("\\]", flags);
    pattern = regex_replace(pattern,re,"\\]\\)");
    re.assign("[[^\\]]*\\.[^\\]]*\\]", flags);
    pattern = regex_replace(pattern,re,".");
    re.assign("(?<!\\\\)([0-9])", flags);
    pattern = regex_replace(pattern,re,"\\[$1~\\]");
    re.assign("(\\\\[0-9]+)", flags);
    pattern = regex_replace(pattern,re,"\\(\\?\\:~\\|$1\\)");
    
    re.assign("\\[((\\[[^\\]]*\\])*)\\]", flags);
    while (regex_search(pattern,what,re)) {
      string src = what[1];
      DBG cout << "src is " << src << endl;
      
      re.assign("[\\[\\]]", flags);
      string dest = regex_replace(src,re,"");
      
      re.assign("\\[\\Q" + src + "\\E\\]", flags);
      pattern = regex_replace(pattern,re,"\\["+dest+"\\]");
    }
    
    re.assign("(\\\\([0-9]))", flags);
    while (regex_search(pattern,what,re)) {
      string num = what[2];
      string num2 = str(format("%d") % (1+atoi(num.c_str())));
      re.assign("\\\\" + num, flags);
      pattern = regex_replace(pattern,re,"^" + num2);
    }
    
    re.assign("\\^", flags);
    pattern = regex_replace(pattern,re,"\\\\");
  } catch(boost::regex_error& regErr) {
    cerr << "regular expression failed: " << regErr.what() << endl;
    pattern = "(0)*";
  }

  DBG cout << "localized pattern " << pattern << endl;

  return pattern;
}



/*
  operates on normalized and localized sequences and patterns only
  
  alphabet can be modified
 */
list<string> extendSequence(const list<string>& seq, 
			    string pat) {

  list<string> results;

  std::string result;
  boost::regex_constants::syntax_option_type flags = 
    boost::regex_constants::perl;
  boost::regex re;
  boost::smatch what;

  pat = "^" + pat;
  re.assign(pat, flags);
  for (list<string>::const_iterator it = seq.begin(); it!=seq.end(); it++) {
    string activeSeq = *it;

    Normalizer n;
    n.norm(activeSeq);
    string input = n.getInput() + "z";

    for (int i=0; i<input.length(); i++) {
      string ref = activeSeq+input[i]+"~~~~~~~~~~~~~~~~~~";
      DBG cout << "pattern " << pat << " against " << ref << endl;
      try {
	if (regex_search(ref,what,re)) {
	  string m = what[0];
	  if (m.length()>activeSeq.length()) {
	    string bit;
	    bit += input[i];
	    string ext = activeSeq + n.norm(bit);
	    DBG cout << "  got something " << ext << endl;
	    results.push_back(string(ext));
	  }
	}
      } catch(boost::regex_error& regErr) {
	cerr << "regular expression failed: " << regErr.what() << endl;
      }
    }
  }

  return results;
}



std::string extendSequence(std::string seq, std::string pattern, int len) {

  srand (std::time(0));

  checkSequence(seq);

  Normalizer n;
  int pruneLen = 100;

  seq = n.norm(seq);
  pattern = n.norm(pattern);
  string refPattern = pattern;
  pattern = localizePattern(pattern);

  DBG cout << "extend " << seq << " with " << pattern << endl;

  n.add("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  list<string> lst;
  lst.push_back(seq);
  for (int k=0; k<len; k++) {
    lst = extendSequence(lst,pattern);

    
    list<string> lst2;
    for (list<string>::const_iterator it = lst.begin(); it!=lst.end(); 
	 it++) {
      string nextSeq = *it;
      string nextPat = refPattern;
      if (nextSeq.length()<=10) {
	nextPat = sequenceToPattern(nextSeq);
      }
      if (nextPat==refPattern) {
	lst2.push_back(nextSeq);
      } else {
	DBG cout << nextSeq << ": " << "mismatch " << nextPat << " versus " << refPattern << endl;
      }
    }
    if (lst2.size()>0) {
      lst = lst2;
    }


    if (lst.size()>pruneLen) {
      DBG cout << "NEED TO PRUNE" << endl;
      vector<string> v(lst.begin(),lst.end());
      random_shuffle(v.begin(),v.end());
      lst.clear();
      lst = list<string>(v.begin(),v.begin()+pruneLen);
      //lst.erase((++(++(lst.begin()))),lst.end());
    }
    DBG {
      cout << "possibilities: " << endl;
      for (list<string>::const_iterator it = lst.begin(); it!=lst.end(); 
	   it++) {
	cout << " -- " << n.denorm(*it) << endl;
      }
    }
  }

  for (list<string>::const_iterator it = lst.begin(); it!=lst.end(); 
       it++) {
    DBG cout << "  final possibility " << n.denorm(*it) << endl;
  }
  vector<string> v(lst.begin(),lst.end());
  random_shuffle(v.begin(),v.end());
  if (v.size()>0) {
    string result = n.denorm(v[0]);
    result = result.substr(seq.length(),result.length());
    return result;
  }

  return "";
}




static std::string submain(std::string sequence)
{
  std::string result;
  boost::regex_constants::syntax_option_type flags = 
    boost::regex_constants::perl; //boost::regex_constants::basic;
  boost::regex re;
  boost::smatch what;

   try{
     std::string pattern = str(boost::format("^%s (.*)") % sequence);

     re.assign(pattern, flags);

     std::string name = 
       str(boost::format("sequence_hints/ref%02d.txt") % sequence.length());

     std::ifstream is(name.c_str());
     if(!is.good()) {
       std::cerr << "Unable to open file " << name << std::endl;
       exit(1);
     }
     std::string line;
     while(std::getline(is, line)) {
       if (boost::regex_search(line, what, re)) {
	 if (what.size()>=2) {
	   result = what[1];
	   DBG cout << "RESULT is " << result << endl;
	   break;
	 }
       }
     }
   }
   catch(const std::exception& e)
   {
     std::cerr << "exception: " << e.what() << std::endl;
   }

   return result;
}


std::string sequenceToPattern(std::string seq) {
  checkSequence(seq);

  Normalizer n;

  std::string sub = "";
  if (seq.length()>10) {
    sub = seq.substr(0,seq.length()-10);
    seq = seq.substr(seq.length()-10,seq.length());
  }

  string pat = n.denorm(submain(n.norm(seq)));
  pat = sub + pat;

  return pat;
}


vector<int> extendSequence(const vector<int>& seq, 
			   int ct) {
  vector<int> result;
  Normalizer n;
  int at = 0;
  map<int,char> lower;
  map<char,int> revLower;

  for (vector<int>::const_iterator it = seq.begin();
       it!=seq.end(); it++) {
    int v = *it;
    if (lower.find(v)==lower.end()) {
      char ch = n.toChar(at);
      if (ch==-1) {
	printf("Too many units in symbolic sequence\n");
      }
      lower[v] = ch;
      revLower[ch] = v;
      at++;
    }
  }
  if (at>10) {
    // alternate mode: just look for best prior
    int hist = seq.size()-1;
    for (int i=0; i<ct; i++) {
      int bestHits = -1;
      int bestAt = 0;
      for (int j=0; j<seq.size(); j++) {
	int hits = 0;
	for (int k=0; k<=hist; k++) {
	  if (j-k-1<0) {
	    break;
	  }
	  if (seq[j-k-1]!=seq[hist-k]) {
	    break;
	  }
	  hits++;
	}
	if (hits>bestHits) {
	  bestAt = j;
	  bestHits = hits;
	}
      }
      hist = bestAt;
      result.push_back(seq[hist]);
    }
    cerr << "Did alternative search" << endl;
    cerr << "Got: ";
    for (int i=0; i<result.size(); i++) {
      cerr << result[i] << " ";
    }
    cerr << endl;
    return result;
  }

  string strSeq;
  for (vector<int>::const_iterator it = seq.begin();
       it!=seq.end(); it++) {
    strSeq += lower[*it];
  }
  string pat = sequenceToPattern(strSeq);
  string ext = extendSequence(strSeq,pat,ct);
  map<char,int> novel;
  int novelAt = 1;
  for (int i=0; i<ext.length(); i++) {
    char ch = ext[i];
    int v = 0;
    if (revLower.find(ch)==revLower.end()) {
      if (novel.find(ch)==novel.end()) {
	novel[ch] = novelAt;
	novelAt++;
      }
      v = -novel[ch];
    } else {
      v = revLower[ch];
    }
    result.push_back(v);
  }
  return result;
}


