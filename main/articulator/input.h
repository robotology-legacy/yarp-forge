#ifndef __INPUT_H
#define __INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h" // For TRMParameters

TRMData *parseInputFile(const char *inputFile);
void addInput(TRMData *data, double glotPitch, double glotVol, double aspVol, double fricVol,
              double fricPos, double fricCF, double fricBW, double *radius,
              double velum);

double glotPitchAt(INPUT *ptr);
double glotVolAt(INPUT *ptr);
double *radiiAt(INPUT *ptr);
double radiusAtRegion(INPUT *ptr, int region);
double velumAt(INPUT *ptr);
double aspVolAt(INPUT *ptr);
double fricVolAt(INPUT *ptr);
double fricPosAt(INPUT *ptr);
double fricCFAt(INPUT *ptr);
double fricBWAt(INPUT *ptr);

void printControlRateInputTable(TRMData *data);

#ifdef __cplusplus
};
#endif

#endif
