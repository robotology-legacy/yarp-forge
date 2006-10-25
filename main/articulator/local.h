
#ifdef rint
#undef rint
#endif
#define rint(x) ((long int)(x))


#ifdef isnan
#undef isnan
#endif
#define isnan(x) (0)
#ifdef isinf
#undef isinf
#endif
#define isinf(x) (0)
