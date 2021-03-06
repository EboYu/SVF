/*
 * @file: DDAPass.h
 * @author: Yulei Sui
 * @date: 01/07/2014
 * @version: 1.0
 *
 */


#ifndef DDAPASS_H_
#define DDAPASS_H_

#include <llvm/Support/CBindingWrapping.h>
#include "SVF-C/Types.h"
#include "MemoryModel/PointerAnalysis.h"
#include "DDA/DDAClient.h"
#include "Util/SCC.h"
#include "Util/BasicTypes.h"

/*!
 * Demand-Driven Pointer Analysis.
 * This class performs various pointer analysis on the given module.
 */
class DDAPass: public ModulePass {

public:
    /// Pass ID
    static char ID;
    typedef SCCDetection<SVFG*> SVFGSCC;
    typedef std::set<const SVFGEdge*> SVFGEdgeSet;
    typedef std::vector<PointerAnalysis*> PTAVector;

    DDAPass() : ModulePass(ID), _pta(NULL), _client(NULL) {}
    ~DDAPass();

    virtual inline void getAnalysisUsage(AnalysisUsage &au) const {
        // declare your dependencies here.
        /// do not intend to change the IR in this pass,
        au.setPreservesAll();
    }

    virtual inline void* getAdjustedAnalysisPointer(AnalysisID id) {
        return this;
    }

    /// Interface expose to users of our pointer analysis, given Location infos
    virtual inline AliasResult alias(const MemoryLocation &LocA, const MemoryLocation &LocB) {
        return alias(LocA.Ptr, LocB.Ptr);
    }

    /// Interface expose to users of our pointer analysis, given Value infos
    virtual AliasResult alias(const Value* V1,	const Value* V2);

    virtual bool runOnModuleCXT(SVFModule module);

    /// We start from here
    virtual bool runOnModule(SVFModule module);

    /// We start from here
    virtual bool runOnModule(Module& module) {
        return runOnModule(module);
    }

    /// Select a client
    virtual void selectClient(SVFModule module);

    /// Pass name
    virtual inline StringRef getPassName() const {
        return "DDAPass";
    }

    /// Print queries' pts
    void printQueryPTS(std::string filePath);
    /// Create pointer analysis according to specified kind and analyze the module.
    void runPointerAnalysis(SVFModule module, u32_t kind);
    /// Initialize queries for DDA
    void answerQueries(PointerAnalysis* pta);
    /// Context insensitive Edge for DDA
    void initCxtInsensitiveEdges(PointerAnalysis* pta, const SVFG* svfg,const SVFGSCC* svfgSCC, SVFGEdgeSet& insensitveEdges);
    /// Return TRUE if this edge is inside a SVFG SCC, i.e., src node and dst node are in the same SCC on the SVFG.
    bool edgeInSVFGSCC(const SVFGSCC* svfgSCC,const SVFGEdge* edge);
    /// Return TRUE if this edge is inside a SVFG SCC, i.e., src node and dst node are in the same SCC on the SVFG.
    bool edgeInCallGraphSCC(PointerAnalysis* pta,const SVFGEdge* edge);
    /// Dump the map between node id and pointer
    void dumpNodeID(std::string filePath);

    void collectCxtInsenEdgeForRecur(PointerAnalysis* pta, const SVFG* svfg,SVFGEdgeSet& insensitveEdges);
    void collectCxtInsenEdgeForVFCycle(PointerAnalysis* pta, const SVFG* svfg,const SVFGSCC* svfgSCC, SVFGEdgeSet& insensitveEdges);

    PointerAnalysis* _pta;	///<  pointer analysis to be executed.
    DDAClient* _client;		///<  DDA client used
    
};
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(DDAPass, SVFDDAPass)

#endif /* DDAPASS_H_ */
