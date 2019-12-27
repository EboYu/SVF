#ifndef SVF_C_UTILS_H
#define SVF_C_UTILS_H

#include "SVF-C/Types.h"
#include <vector>
#include <string>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

    SVFDDAPass DDAPassCreate();
    void DDAPassDispose(SVFDDAPass M);
    void DDAPassRunOnModule(SVFDDAPass P,SVFSVFModule M);
    void DDAPassDumpNodeID(SVFDDAPass M, const char* filePath);
    void DDAPassPrintQueryPTS(SVFDDAPass M, const char* filePath);

    //SVFSVFModule SVFModuleCreate();
    SVFSVFModule SVFModuleCreate(const std::vector<std::string> &moduleNameVec);
    void SVFSVFModuleDispose(SVFSVFModule M);
    void SVFDumpModulesToFile(SVFSVFModule M, const char* suffix);


    void CLParseCommandLineOptions(int arg_num, char **arg_value, const char * cmd);
    int SVFProcessArguments(int argc, char **argv, char **arg_value, std::vector<std::string>* moduleNameVec);
    
    
#ifdef __cplusplus
}
#endif

#endif