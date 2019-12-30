#ifndef SVF_C_UTILS_H
#define SVF_C_UTILS_H

#include "SVF-C/Types.h"
#include <vector>
#include <string>
#include <map>
#include <set>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

    SVFDDAPass DDAPassCreate();
    void DDAPassDispose(SVFDDAPass M);
    void DDAPassRunOnModule(SVFDDAPass P,SVFSVFModule M);
    void DDAPassDumpNodeID(SVFDDAPass M, const char* filePath);
    void DDAPassPrintQueryPTS(SVFDDAPass M, const char* filePath);
    void DDAPassInitilize(SVFDDAPass P,SVFSVFModule M);
    void DDAPassAnswerQuery(SVFDDAPass M, const char* query);
    const std::map<long, CPAGNode_t>& DDAPassExtractAllValidPtrs(SVFDDAPass M);
    void DDAPassPointsToSet(SVFDDAPass M,std::map<long, set<long>> &nodePTSSet);
    long DDAPassGetParentNode(SVFDDAPass M,long nodeID);

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