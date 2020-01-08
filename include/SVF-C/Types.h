#ifndef SVF_C_TYPES_H
#define SVF_C_TYPES_H

using namespace std;
#ifdef __cplusplus
extern "C" {
#endif

    typedef int SVFBool;
    

    typedef struct OpaqueDDAPass *SVFDDAPass; 
    typedef struct OpaqueSVFModule *SVFSVFModule;
    // typedef struct OpaquePAGNode *SVFPAGNode;

    typedef struct CPAGNode_s{
        long nodeID;
        bool isTLPointer;	/// true === top-level pointer, false ===  address-taken pointer
        const char *functionName;
        int irID;
        const char *pointerName;
        const char *instruction;
        int variableType;
        int lineNum;
    }CPAGNode_t;

    bool starting();
    void performSVF(int argc, char ** argv);
    
#ifdef __cplusplus
}
#endif

#endif