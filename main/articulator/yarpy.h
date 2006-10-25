#ifndef YARPY_INC
#define YARPY_INC

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"

  void yarpy();

  void setParams(TRMParameters *params);

  double getParam(int x, int y);

  double getTime();

  void setParamTarget(int x, int y, double v);

#ifdef __cplusplus
};
#endif

#endif
