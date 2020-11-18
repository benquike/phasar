#include <iostream>
#include <set>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/Logger.h"
#include "GLAnalysis.h"

using namespace std;

namespace psr {

  struct TestFF:FlowFunction<GLAnalysisProblem::d_t> {
    const llvm::Instruction *inst;
    const llvm::Value *v;
    GLAnalysisProblem *p;
    string from;
    int close_data_flow;

    TestFF(const llvm::Instruction *inst,
           const llvm::Value *v,
           const string &from = "",
           int close_data_flow = 0,
           GLAnalysisProblem *p=nullptr): inst(inst),
                                     v(v),
                                     p(p),
                                     from(from),
                                     close_data_flow(close_data_flow) { }

    string get_const_string_val(const llvm::Value *val) {
      if (const llvm::ConstantExpr *ge = llvm::dyn_cast<llvm::ConstantExpr>(val)) {
        llvm::Value *v = ge->getOperand(0);
        if (llvm::GlobalVariable *gv = llvm::dyn_cast<llvm::GlobalVariable>(v)) {
          if (gv->hasInitializer()) {
            llvm::Constant *c = gv->getInitializer();
            // llvm::outs() << "2: " << *c << "\n";
            if (llvm::ConstantDataArray *ca = llvm::dyn_cast<llvm::ConstantDataArray>(c)) {
              // llvm::outs() << ca->getAsString().str() << "\n";
              return ca->getAsString().str();
            } else {
              llvm::outs() << "Not ConstantArray\n";
            }
          } else {
            llvm::outs() << "Not Having Initializer\n";
          }
        } else {
          llvm::outs() << "Not GlobalVariable\n";
        }
      } else if (auto li = llvm::dyn_cast<llvm::LoadInst>(val)) {
        const llvm::Value *v = li->getPointerOperand();
        llvm::outs()  << *v << "\n";

        if (p != nullptr) {
          auto pt_info = p->get_pointsto_info();
          auto possible_sites = pt_info->getPointsToSet(v);
          for (auto x : *possible_sites) {
            llvm::outs() << " -- site: " << *x << "\n";
          }
        }
      }
      return "";
    }


    set<GLAnalysisProblem::d_t>
    computeTargets(GLAnalysisProblem::d_t source) override {
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
      //             << "***** 0 Function called, callee:" << dest_fun->getName().str());
      // llvm::outs() << "******  " << *inst << " : " << from << " *** "  << source << "\n";
      //

      if (v) {
        if (auto *dest_func = llvm::dyn_cast<llvm::Function>(v)) {
          if (p != nullptr && dest_func->getName().str() == p->get_log_func_name()) {
            llvm::outs() << " a call to log function is seen\n";
            if (inst) {
              if (const llvm::CallInst *cic = llvm::dyn_cast<llvm::CallInst>(inst)) {
                llvm::CallInst *ci = const_cast<llvm::CallInst*>(cic);
                llvm::CallSite cs(ci);
                if (cs.getInstruction()) {
                  // llvm::outs() << "Arguments size:" << cs.arg_size() << "\n";
                  // for (unsigned i = 0; i < cs.arg_size(); i ++) {
                  //   llvm::outs() << "argument " << i <<  ":" << *cs.getArgOperand(i) << "\n";
                  // }
                  // get const string values
                  // get_const_string_val(cs.getArgOperand(2));
                  // get_const_string_val(cs.getArgOperand(2));
                  const string &log_msg = get_const_string_val(cs.getArgOperand(3));
                  if (log_msg != "") {
                    llvm::outs() << "LOG MSG:" << log_msg << "\n";
                  } else {
                    llvm::outs() << "LOG MSG: EMPTY:" << *inst << "\n";
                  }
                }
              }  // if cic
            } // if (inst)
          }
        }
      }

      if (close_data_flow) {
        return {};
      }

      return {source};
    }
  };

  GLAnalysisProblem::GLAnalysisProblem(ProjectIRDB *db,
                                       LLVMTypeHierarchy *type_hierarchy,
                                       LLVMBasedICFG *icfg,
                                       LLVMPointsToInfo *points_to,
                                       string entry_point):
    IFDSTabulationProblem(db, type_hierarchy, icfg, points_to),
    _entry_point(entry_point) {

    // why is this needed?
    ZeroValue = createZeroValue();
  }

  GLAnalysisProblem::FlowFunctionPtrType
  GLAnalysisProblem::getNormalFlowFunction(n_t curr,
                                           n_t succ) {

    // return Identity<GLAnalysisProblem::d_t>::getInstance();
    //
    return make_shared<TestFF>(curr, nullptr, __func__);
  }

  GLAnalysisProblem::FlowFunctionPtrType
  GLAnalysisProblem::getCallFlowFunction(n_t call_stmt,
                                         f_t dest_fun) {

    auto d_f = llvm::dyn_cast<llvm::Function>(dest_fun);

    int close = 0;
    if (dest_fun->isIntrinsic() ||
        _log_func_name == dest_fun->getName().str()) {
      close = 1;
    }

    return make_shared<TestFF>(call_stmt, dest_fun, __func__, close);
  }

  GLAnalysisProblem::FlowFunctionPtrType
  GLAnalysisProblem::getRetFlowFunction(n_t call_site,
                                        f_t callee_fun,
                                        n_t exit_stmt,
                                        n_t ret_site) {

    return make_shared<TestFF>(call_site, nullptr, __func__);
  }


  GLAnalysisProblem::FlowFunctionPtrType
  GLAnalysisProblem::getCallToRetFlowFunction(n_t call_site,
                                              n_t ret_site,
                                              set<f_t> callees) {
    return make_shared<TestFF>(call_site, nullptr, __func__);
  }


  GLAnalysisProblem::FlowFunctionPtrType
  GLAnalysisProblem::getSummaryFlowFunction(n_t call_stmt,
                                            f_t dest_fun) {
    // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
    //               << "***** Function3 called, callee:" << dest_fun->getName().str());
    // return Identity<GLAnalysisProblem::d_t>::getInstance();
    // return make_shared<TestFF>(call_stmt, nullptr, __func__);

    // ignore a special function now
    // string sp_name = "_ZN5blink20MakeGarbageCollectedINS_11WebGLShaderEJPNS_25WebGLRenderingContextBaseERjEEEPT_DpOT0_";
    if (dest_fun->isIntrinsic()
        || _log_func_name == dest_fun->getName().str()
        /* || dest_fun->getName().str() == sp_name*/ ) {
      // return Identity<GLAnalysisProblem::d_t>::getInstance();
      return make_shared<TestFF>(call_stmt, dest_fun, __func__, 0, this);
    }

    return nullptr;
  }

  GLAnalysisProblem::d_t
  GLAnalysisProblem::createZeroValue() const {
    return LLVMZeroValue::getInstance();
  }


  bool GLAnalysisProblem::
  isZeroValue(GLAnalysisProblem::d_t d) const {
    return false;
  }

  map<GLAnalysisProblem::n_t, set<GLAnalysisProblem::d_t>>
  GLAnalysisProblem::initialSeeds() {
    map<GLAnalysisProblem::n_t, set<GLAnalysisProblem::d_t>> res;

    res.insert(make_pair(&ICF->getFunction(_entry_point)->front().front(),
                         set<GLAnalysisProblem::d_t>({getZeroValue()})));
    return res;
  }

  void GLAnalysisProblem::
  printNode(ostream &os,
            GLAnalysisProblem::n_t t) const {

  }

  void GLAnalysisProblem::
  printDataFlowFact(ostream &os,
                    GLAnalysisProblem::d_t d) const {

  }

  void GLAnalysisProblem::
  printFunction(ostream &os,
                GLAnalysisProblem::f_t f) const {

  }

  void GLAnalysisProblem::
  emitTextReport(const SolverResults<GLAnalysisProblem::n_t,
                 GLAnalysisProblem::d_t, BinaryDomain> &sr,
                 ostream &os) {

  }
}
