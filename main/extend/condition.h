#ifndef PATTERN_CONDITION_INC
#define PATTERN_CONDITION_INC

#include <vector>
#include <list>
#include <string>
#include <stdio.h>

#include <boost/shared_ptr.hpp>

#include "Unit.h"

#include "Histogram.h"



class Unit : public IUnit {
private:
  boost::shared_ptr<IUnit> content;
public:
  Unit(IUnit *unit) : content(unit) {
  }

  Unit() {
  }

  void set(IUnit *unit) {
    content = boost::shared_ptr<IUnit>(unit);
  }

  virtual int size() const {
    return content->size();
  }

  virtual double get(int index) const {
    return content->get(index);
  }

  virtual double predict(int index) {
    return content->predict(index);
  }

  virtual bool extend(int len) {
    return content->extend(len);
  }

  virtual void copy(const IUnit& alt) {
    content->copy(alt);
  }

  virtual int futureSize() const {
    return content->futureSize();
  }

};


class Sequence : public IUnit {
private:
  std::vector<double> past;
  std::vector<double> future;
public:
  void clear() {
    past.clear();
    future.clear();
  }

  void add(double past) {
      this->past.push_back(past);
  }

  void add(const std::vector<double>& past) {
    for (std::vector<double>::const_iterator it=past.begin(); it!=past.end();
	 it++) {
      add(*it);
    }
  }

  virtual int size() const {
    return past.size();
  }

  virtual int futureSize() const {
    return future.size();
  }

  virtual double get(int index) const {
    return past[index];
  }

  virtual double predict(int index) {
    if (extend(index+1)) {
      return future[index];
    }
    return 0;
  }

  virtual bool extend(int len);

  virtual void copy(const IUnit& alt) {
    clear();
    for (int i=0; i<alt.size(); i++) {
      add(alt[i]);
    }
  }

  void takeFuture(IUnit& alt, int len) {
    bool ok = alt.extend(len);
    clear();
    if (ok) {
      for (int i=0; i<len; i++) {
	add(alt.predict(i));
      }
    }
  }
};




class SymbolicSequence : public IUnit {
private:
  std::vector<int> past;
  std::vector<int> future;
public:
  void clear() {
    past.clear();
    future.clear();
  }

  virtual int futureSize() const {
    return future.size();
  }

  void add(double past) {
      this->past.push_back((int)(past+0.5));
  }

  virtual int size() const {
    return past.size();
  }

  virtual double get(int index) const {
    return past[index];
  }


  int getSym(int index) const {
    return past[index];
  }

  virtual double predict(int index) {
    if (extend(index+1)) {
      return future[index];
    }
    return 0;
  }

  virtual bool extend(int len);

  virtual void copy(const IUnit& alt) {
    clear();
    for (int i=0; i<alt.size(); i++) {
      add(alt[i]);
    }
  }
};



class ConstantSequence : public IUnit {
private:
  Histogram h;
  int fsize;
public:
  ConstantSequence() {
    fsize = 0;
  }

  virtual int futureSize() const {
    return fsize;
  }

  void clear() {
    h.clear();
  }

  void add(double past) {
    h.add(past);
  }

  virtual int size() const {
    return h.getLength();
  }

  virtual double get(int index) const {
    return h.getPast(index);
  }

  virtual double predict(int index);

  virtual bool extend(int len) {
    if (len>fsize) {
      fsize = len;
    }
    return true;
  }

  virtual void copy(const IUnit& alt) {
    clear();
    for (int i=0; i<alt.size(); i++) {
      add(alt[i]);
    }
  }
};


class Piece {
public:
  double mean, dev;
  double len;
  Unit lengthUnit;
};


class PiecewiseSequence : public IUnit {
private:
  SymbolicSequence sym;
  std::vector<Piece> parts;
  Piece whole;
  std::vector<double> future;
  int places, scale;
  double meanLength;

  bool extendParts(int len, int pad);
public:
  virtual void copy(const IUnit& alt) {
    apply(alt);
  }

  void apply(const IUnit& unit);


  virtual int size() const {
    return sym.size();
  }

  virtual int futureSize() const {
    return future.size();
  }

  virtual double get(int index) const {
    return sym[index];
  }

  virtual double predict(int index) {
    if (extend(index+1)) {
      return future[index];
    }
    return 0;
  }

  virtual bool extend(int len);
};


#endif


