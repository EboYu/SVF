#ifndef SVF_C_UTILS_H
#define SVF_C_UTILS_H

#include "SVF-C/Types.h"

#ifdef __cplusplus
extern "C" {
#endif

    void SVFProcessArguments(int argc, char **argv, int &arg_num, char **arg_value,std::vector<std::string> &moduleNameVec);
    
#ifdef __cplusplus
}
#endif

#endif