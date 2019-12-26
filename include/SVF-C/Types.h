#ifndef SVF_C_TYPES_H
#define SVF_C_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

    typedef int SVFBool;

    typedef struct OpaqueDDAPass *SVFDDAPass; 
    typedef struct OpaqueSVFModule *SVFSVFModule;
    bool starting();
    void performSVF(int argc, char ** argv);
    
#ifdef __cplusplus
}
#endif

#endif