/*
 * @file: DDAPass.cpp
 * @author: Yulei Sui
 * @date: 01/07/2014
 */


#include "MemoryModel/PointerAnalysis.h"
#include "DDA/DDAPass.h"
#include "DDA/FlowDDA.h"
#include "DDA/ContextDDA.h"
#include "DDA/DDAClient.h"
#include "SVF-C/Types.h"
#include "SVF-C/Utils.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <limits.h>

using namespace SVFUtil;

char DDAPass::ID = 0;

static llvm::cl::opt<unsigned> maxPathLen("maxpath",  llvm::cl::init(200000),
                                    llvm::cl::desc("Maximum path limit for DDA"));

static llvm::cl::opt<unsigned> maxContextLen("maxcxt",  llvm::cl::init(200000),
                                       llvm::cl::desc("Maximum context limit for DDA"));

static llvm::cl::opt<string> userInputQuery("query",  llvm::cl::init("all"),
                                      llvm::cl::desc("Please specify queries by inputing their pointer ids"));

static llvm::cl::opt<bool> insenRecur("inrecur", llvm::cl::init(false),
                                llvm::cl::desc("Mark context insensitive SVFG edges due to function recursions"));

static llvm::cl::opt<bool> insenCycle("incycle", llvm::cl::init(false),
                                llvm::cl::desc("Mark context insensitive SVFG edges due to value-flow cycles"));

static llvm::cl::opt<bool> printCPts("cpts", llvm::cl::init(false),
                               llvm::cl::desc("Dump conditional points-to set "));

static llvm::cl::opt<bool> printQueryPts("print-query-pts", llvm::cl::init(false),
                                   llvm::cl::desc("Dump queries' conditional points-to set "));

static llvm::cl::opt<bool> dumpNodeId("dump-map", llvm::cl::init(false),
                                   llvm::cl::desc("Dump the map between node id and pointer"));    

static llvm::cl::opt<bool> WPANUM("wpanum", llvm::cl::init(false),
                            llvm::cl::desc("collect WPA FS number only "));

static llvm::RegisterPass<DDAPass> DDAPA("dda", "Demand-driven Pointer Analysis Pass");

/// register this into alias analysis group
//static RegisterAnalysisGroup<AliasAnalysis> AA_GROUP(DDAPA);

static llvm::cl::bits<PointerAnalysis::PTATY> DDASelected(llvm::cl::desc("Select pointer analysis"),
		llvm::cl::values(
            clEnumValN(PointerAnalysis::FlowS_DDA, "dfs", "Demand-driven flow sensitive analysis"),
            clEnumValN(PointerAnalysis::Cxt_DDA, "cxt", "Demand-driven context- flow- sensitive analysis")
        ));



DDAPass::~DDAPass() {
    // _pta->dumpStat();
    if (_client != NULL)
        delete _client;
}

bool DDAPass::runOnModuleCXT(SVFModule module)
{
    /// initialization for llvm alias analyzer
    //InitializeAliasAnalysis(this, SymbolTableInfo::getDataLayout(&module));
    selectClient(module);
    
    runPointerAnalysis(module, 20);
    
    return false;
}

bool DDAPass::runOnModule(SVFModule module)
{
    /// initialization for llvm alias analyzer
    //InitializeAliasAnalysis(this, SymbolTableInfo::getDataLayout(&module));
    selectClient(module);
    
    for (u32_t i = PointerAnalysis::FlowS_DDA;
            i < PointerAnalysis::Default_PTA; i++) {
        if (DDASelected.isSet(i)){
            runPointerAnalysis(module, i);
        }
            
    }
    
    return false;
}

/// select a client to initialize queries
void DDAPass::selectClient(SVFModule module) {

    if (!userInputQuery.empty()) {
        /// solve function pointer
        if(userInputQuery == "funptr") {
            _client = new FunptrDDAClient(module);
        }
        /// allow user specify queries
        else {
            _client = new DDAClient(module);
            if (userInputQuery != "all") {
                u32_t buf; // Have a buffer
                stringstream ss(userInputQuery); // Insert the user input string into a stream
                while (ss >> buf)
                    _client->setQuery(buf);
            }
        }
    }
    else {
        assert(false && "Please specify query options!");
    }

    _client->initialise(module);
}

/// Create pointer analysis according to specified kind and analyze the module.
void DDAPass::runPointerAnalysis(SVFModule module, u32_t kind)
{
    //set max path limit
    VFPathCond::setMaxPathLen(maxPathLen);
    //set max context limit
    ContextCond::setMaxCxtLen(maxContextLen);

    /// Initialize pointer analysis.
    switch (kind) {
    case PointerAnalysis::Cxt_DDA: {
        _pta = new ContextDDA(module, _client);
        break;
    }
    case PointerAnalysis::FlowS_DDA: {
        _pta = new FlowDDA(module, _client);
        break;
    }
    default:
        outs() << "This pointer analysis has not been implemented yet.\n";
        break;
    }

    if(WPANUM) {
        _client->collectWPANum(module);
    }
    else {
        ///initialize
        _pta->initialize(module);
        ///compute points-to
        answerQueries(_pta);
        ///finalize
        _pta->finalize();
        
        if(printCPts)
            _pta->dumpCPts();

        if (_pta->printStat())
            _client->performStat(_pta);

    
        if (printQueryPts){
            std::string filePath = module.generateFilePath("result.txt");
            printQueryPTS(filePath);
        }
            
        if (dumpNodeId){
            std::string filePath = module.generateFilePath("pointermap.txt");
            dumpNodeID(filePath);
        }
    }
}

/*!
 * Initialize queries
 */
void DDAPass::answerQueries(PointerAnalysis* pta) {

    DDAStat* stat = static_cast<DDAStat*>(pta->getStat());
    u32_t vmrss = 0;
    u32_t vmsize = 0;
    SVFUtil::getMemoryUsageKB(&vmrss, &vmsize);
    stat->setMemUsageBefore(vmrss, vmsize);

    _client->answerQueries(pta);

    vmrss = vmsize = 0;
    SVFUtil::getMemoryUsageKB(&vmrss, &vmsize);
    stat->setMemUsageAfter(vmrss, vmsize);
}


/*!
 * Initialize context insensitive Edge for DDA
 */
void DDAPass::initCxtInsensitiveEdges(PointerAnalysis* pta, const SVFG* svfg,const SVFGSCC* svfgSCC, SVFGEdgeSet& insensitveEdges) {
    if(insenRecur)
        collectCxtInsenEdgeForRecur(pta,svfg,insensitveEdges);
    else if(insenCycle)
        collectCxtInsenEdgeForVFCycle(pta,svfg,svfgSCC,insensitveEdges);
}

/*!
 * Whether SVFG edge in a SCC cycle
 */
bool DDAPass::edgeInSVFGSCC(const SVFGSCC* svfgSCC,const SVFGEdge* edge) {
    return (svfgSCC->repNode(edge->getSrcID()) == svfgSCC->repNode(edge->getDstID()));
}

/*!
 *  Whether call graph edge in SVFG SCC
 */
bool DDAPass::edgeInCallGraphSCC(PointerAnalysis* pta,const SVFGEdge* edge) {
    const BasicBlock* srcBB = edge->getSrcNode()->getBB();
    const BasicBlock* dstBB = edge->getDstNode()->getBB();

    if(srcBB && dstBB)
        return pta->inSameCallGraphSCC(srcBB->getParent(),dstBB->getParent());

    assert(edge->isRetVFGEdge() == false && "should not be an inter-procedural return edge" );

    return false;
}

/*!
 * Mark insensitive edge for function recursions
 */
void DDAPass::collectCxtInsenEdgeForRecur(PointerAnalysis* pta, const SVFG* svfg,SVFGEdgeSet& insensitveEdges) {

    for (SVFG::SVFGNodeIDToNodeMapTy::const_iterator it = svfg->begin(),eit = svfg->end(); it != eit; ++it) {

        SVFGEdge::SVFGEdgeSetTy::const_iterator edgeIt = it->second->InEdgeBegin();
        SVFGEdge::SVFGEdgeSetTy::const_iterator edgeEit = it->second->InEdgeEnd();
        for (; edgeIt != edgeEit; ++edgeIt) {
            const SVFGEdge* edge = *edgeIt;
            if(edge->isCallVFGEdge() || edge->isRetVFGEdge()) {
                if(edgeInCallGraphSCC(pta,edge))
                    insensitveEdges.insert(edge);
            }
        }
    }
}

/*!
 * Mark insensitive edge for value-flow cycles
 */
void DDAPass::collectCxtInsenEdgeForVFCycle(PointerAnalysis* pta, const SVFG* svfg,const SVFGSCC* svfgSCC, SVFGEdgeSet& insensitveEdges) {

    std::set<NodePair> insensitvefunPairs;

    for (SVFG::SVFGNodeIDToNodeMapTy::const_iterator it = svfg->begin(),eit = svfg->end(); it != eit; ++it) {

        SVFGEdge::SVFGEdgeSetTy::const_iterator edgeIt = it->second->InEdgeBegin();
        SVFGEdge::SVFGEdgeSetTy::const_iterator edgeEit = it->second->InEdgeEnd();
        for (; edgeIt != edgeEit; ++edgeIt) {
            const SVFGEdge* edge = *edgeIt;
            if(edge->isCallVFGEdge() || edge->isRetVFGEdge()) {
                if(this->edgeInSVFGSCC(svfgSCC,edge)) {

                    const BasicBlock* srcBB = edge->getSrcNode()->getBB();
                    const BasicBlock* dstBB = edge->getDstNode()->getBB();

                    if(srcBB && dstBB) {
                        NodeID src = pta->getPTACallGraph()->getCallGraphNode(srcBB->getParent())->getId();
                        NodeID dst = pta->getPTACallGraph()->getCallGraphNode(dstBB->getParent())->getId();
                        insensitvefunPairs.insert(std::make_pair(src,dst));
                        insensitvefunPairs.insert(std::make_pair(dst,src));
                    }
                    else
                        assert(edge->isRetVFGEdge() == false && "should not be an inter-procedural return edge" );
                }
            }
        }
    }

    for(SVFG::SVFGNodeIDToNodeMapTy::const_iterator it = svfg->begin(),eit = svfg->end(); it != eit; ++it) {
        SVFGEdge::SVFGEdgeSetTy::const_iterator edgeIt = it->second->InEdgeBegin();
        SVFGEdge::SVFGEdgeSetTy::const_iterator edgeEit = it->second->InEdgeEnd();
        for (; edgeIt != edgeEit; ++edgeIt) {
            const SVFGEdge* edge = *edgeIt;

            if(edge->isCallVFGEdge() || edge->isRetVFGEdge()) {
                const BasicBlock* srcBB = edge->getSrcNode()->getBB();
                const BasicBlock* dstBB = edge->getDstNode()->getBB();

                if(srcBB && dstBB) {
                    NodeID src = pta->getPTACallGraph()->getCallGraphNode(srcBB->getParent())->getId();
                    NodeID dst = pta->getPTACallGraph()->getCallGraphNode(dstBB->getParent())->getId();
                    if(insensitvefunPairs.find(std::make_pair(src,dst))!=insensitvefunPairs.end())
                        insensitveEdges.insert(edge);
                    else if(insensitvefunPairs.find(std::make_pair(dst,src))!=insensitvefunPairs.end())
                        insensitveEdges.insert(edge);
                }
            }
        }
    }
}

/*!
 * Return alias results based on our points-to/alias analysis
 * TODO: Need to handle PartialAlias and MustAlias here.
 */
AliasResult DDAPass::alias(const Value* V1, const Value* V2) {
    PAG* pag = _pta->getPAG();

    /// TODO: When this method is invoked during compiler optimizations, the IR
    ///       used for pointer analysis may been changed, so some Values may not
    ///       find corresponding PAG node. In this case, we only check alias
    ///       between two Values if they both have PAG nodes. Otherwise, MayAlias
    ///       will be returned.
    if (pag->hasValueNode(V1) && pag->hasValueNode(V2)) {
        PAGNode* node1 = pag->getPAGNode(pag->getValueNode(V1));
        if(pag->isValidTopLevelPtr(node1))
            _pta->computeDDAPts(node1->getId());

        PAGNode* node2 = pag->getPAGNode(pag->getValueNode(V2));
        if(pag->isValidTopLevelPtr(node2))
            _pta->computeDDAPts(node2->getId());

        return _pta->alias(V1,V2);
    }

    return llvm::MayAlias;
}

/*!
 * Print queries' pts
 */
void DDAPass::printQueryPTS(std::string filePath) {
    std::ofstream fOut(filePath);
    const NodeSet& candidates = _client->getCandidateQueries();
    for (NodeSet::iterator it = candidates.begin(), eit = candidates.end(); it != eit; ++it) {
        const PointsTo& pts = _pta->getPts(*it);
        fOut << _pta->dumpTxtPts(*it,pts) <<std::endl;
    }
    fOut.close();
}


void DDAPass::dumpNodeID(std::string filePath){
    std::ofstream fOut(filePath);
    std::string str="Pointer and Node ID map \n";
    raw_string_ostream rawstr(str);
    fOut << rawstr.str();
    for (NodeSet::iterator nIter = _pta->getAllValidPtrs().begin();
            nIter != _pta->getAllValidPtrs().end(); ++nIter) {
        const PointsTo& pts = _pta->getPts(*nIter);
        fOut << _pta->dumpTxtPointer(*nIter,pts) << std::endl;
    }
    fOut.close();
}

SVFDDAPass DDAPassCreate(){
    return wrap(new DDAPass());
}

void DDAPassDispose(SVFDDAPass M){
    delete unwrap(M);
}

void DDAPassRunOnModule(SVFDDAPass P,SVFSVFModule M){
    unwrap(P)->runOnModuleCXT(*unwrap(M));
}

void DDAPassInitilize(SVFDDAPass P,SVFSVFModule M){
    DDAPass *dda = unwrap(P);
    SVFModule *module = unwrap(M);
    dda->_client = new DDAClient(*module);
    dda->_client->initialise(*module);
    //set max path limit
    VFPathCond::setMaxPathLen(maxPathLen);
    //set max context limit
    ContextCond::setMaxCxtLen(maxContextLen);

    /// Initialize pointer analysis.
    dda->_pta = new ContextDDA(*module, dda->_client);
    dda->_pta->initialize(*module);
}

// using pointer ids as a query
void DDAPassAnswerQuery(SVFDDAPass M, const char* query){
    DDAPass *dda = unwrap(M);
    dda->_client->freshQuery();
    //setup the client for query
    std::string inputQuery = query;
    if (inputQuery != "all") {
        u32_t buf; // Have a buffer
        stringstream ss(inputQuery); // Insert the user input string into a stream
        while (ss >> buf)
            dda->_client->setQuery(buf);
    }else{
        dda->_client->setSolveAll(true);
    }

    dda->_client->answerQueries(dda->_pta);
    // dda->answerQueries(dda->_pta);
    dda->_pta->finalize();

    outs()<<" dump conditional pts\n";
    ContextDDA* cpta =static_cast<ContextDDA*>(dda->_pta);
    cpta->dumpAllPts();

}


long DDAPassGetParentNode(SVFDDAPass M,long nodeID){
    DDAPass *dda = unwrap(M);
    NodeID id = nodeID;
    const PAGNode* node = dda->_pta->getPAG()->getPAGNode(id);
    assert(SVFUtil::isa<ObjPN>(node) && "need an object node");
    const ObjPN* obj = SVFUtil::cast<ObjPN>(node);
    return (long)dda->_pta->getPAG()->getObjectNode(obj->getMemObj());
}

const std::map<long, CPAGNode_t>& DDAPassExtractAllValidPtrs(SVFDDAPass M){
    return unwrap(M)->_pta->extractAllValidPtrs();
}

void DDAPassPointsToSet(SVFDDAPass M,std::map<long, set<long>> &nodePTSSet){
     DDAPass *dda = unwrap(M);
     std::map<long, set<long>>::iterator itera = nodePTSSet.begin();
     for(PAG::iterator it = dda->_pta->getPAG()->begin(), eit = dda->_pta->getPAG()->end(); it!=eit; it++) {
        NodeID ptr = it->first;
        PointsTo pts = dda->_pta->getPts(it->first);
        if (!pts.empty()) {
            set<long> NodeSet;
            for (PointsTo::iterator pit = pts.begin(), peit = pts.end(); pit != peit; ++pit)
                NodeSet.insert(*pit);
            nodePTSSet.insert(itera, std::pair<long, set<long>>(ptr,NodeSet));
        }
     }
}



void DDAPassDumpNodeID(SVFDDAPass M, const char* filePath){
    std::string path = filePath;
    unwrap(M)->dumpNodeID(path);
}

void DDAPassPrintQueryPTS(SVFDDAPass M, const char* filePath){
    std::string path = filePath;
    unwrap(M)->printQueryPTS(path);
}

void DDAPassHasWeakUpdatesInPath(SVFDDAPass M){
    DDAPass *dda = unwrap(M);
    ContextDDA* cpta =static_cast<ContextDDA*>(dda->_pta);
    cpta->getStrongUpdateStats();
}

void CLParseCommandLineOptions(int arg_num, char **arg_value, const char * cmd){
    std::string command = cmd;
    llvm::cl::ParseCommandLineOptions(arg_num,arg_value,command);
}