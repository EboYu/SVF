#ifndef SUPA_C_TYPES_H
#define SUPA_C_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

typedef int SUPABool;

typedef struct OpaqueDDAPass *SUPADDAPass; 

void performSUPA(int argc, char ** argv);

#ifdef __cplusplus
}
#endif

#endif