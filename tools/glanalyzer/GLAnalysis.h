#ifndef __GLANALYSIS_H_
#define __GLANALYSIS_H_

#include <string>
#include <vector>
#include <set>
#include <map>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

using namespace std;

namespace psr {
  class GLAnalysisProblem
    : public IFDSTabulationProblem<LLVMAnalysisDomainDefault> {
private:
  string _entry_point;
  string _log_func_name;
  vector<string> _log_messages;

public:
  GLAnalysisProblem(ProjectIRDB *db,
                    LLVMTypeHierarchy *type_hierarchy,
                    LLVMBasedICFG *icfg,
                    LLVMPointsToInfo *points_to,
                    string entry_point);
  ~GLAnalysisProblem() = default;

  void set_log_func_name(string &func_name) {
    _log_func_name = func_name;
  }

  const string &get_log_func_name() const {
    return _log_func_name;
  }


  auto get_pointsto_info() {
    return PT;
  }

  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) override;
  FlowFunctionPtrType getCallFlowFunction(n_t call_stmt, f_t dest_fun) override;
  FlowFunctionPtrType getRetFlowFunction(n_t call_site, f_t callee_fun,
                                         n_t exit_stmt, n_t ret_site) override;
  FlowFunctionPtrType getCallToRetFlowFunction(n_t call_site, n_t ret_site,
                                               set<f_t> callees) override;
  FlowFunctionPtrType getSummaryFlowFunction(n_t callStmt,
                                             f_t destFun) override;

  map<n_t, set<d_t>> initialSeeds() override;
  bool isZeroValue(d_t d) const override;

  d_t createZeroValue() const override;
  void printNode(ostream &os, n_t n) const override;
  void printDataFlowFact(ostream &os, d_t d) const override;
  void printFunction(ostream &os, f_t m) const override;
  void emitTextReport(const SolverResults<n_t, d_t, BinaryDomain> &SR,
                      ostream &OS = cout) override;
};
}

#endif // __GLANALYSIS_H_
