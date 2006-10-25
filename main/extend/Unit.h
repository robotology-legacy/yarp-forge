#ifndef SEQ_UNIT_INC
#define SEQ_UNIT_INC

#include <vector>
#include <list>
#include <string>
#include <stdio.h>

#include <boost/shared_ptr.hpp>

class IUnit {
public:
  virtual ~IUnit() {}

  virtual int size() const = 0;

  virtual int futureSize() const = 0;

  virtual double get(int index) const = 0;

  virtual double predict(int index) = 0;

  virtual bool extend(int len) = 0;

  virtual void copy(const IUnit& alt) = 0;

  double operator[] (int index) const {
    return get(index);
  }

  std::string toString() const {
    std::string str = "";
    for (int i=0; i<size(); i++) {
      if (i>0) {
	str += " ";
      }
      char buf[256];
      sprintf(buf,"%g",get(i));
      str += buf;
    }
    return str;
  }

  static IUnit *specialize(const IUnit& source);
};



#endif

