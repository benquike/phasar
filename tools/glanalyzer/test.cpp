#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "boost/filesystem/operations.hpp"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

// for defining a problem

// an example of problem
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSUninitializedVariables.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"

// our code
#include "GLAnalysis.h"

namespace llvm {
class Value;
class Instruction;
class Type;
class Function;
class GlobalVariable;
} // namespace llvm

using namespace psr;
using namespace std;

static void validateParamModule(const std::vector<std::string> &Modules) {
  if (Modules.empty()) {
    throw boost::program_options::error_with_option_name(
        "At least one LLVM target module is required!");
  }

  for (const auto &Module : Modules) {
    boost::filesystem::path ModulePath(Module);
    if (!(boost::filesystem::exists(ModulePath) &&
          !boost::filesystem::is_directory(ModulePath) &&
          (ModulePath.extension() == ".ll" ||
           ModulePath.extension() == ".bc"))) {
      throw boost::program_options::error_with_option_name(
          "LLVM module '" + Module + "' does not exist!");
    }
  }
}


int main(int argc, const char **argv) {

  boost::program_options::options_description options("Test program commandline options");
  options.add_options()
    ("log", "enable logging output")
    ("module,m", boost::program_options::value<vector<string>>()->multitoken()\
     ->zero_tokens()->composing()->notifier(&validateParamModule), "Path(s) to IR Module file(s)")
    ("entry-points,e", boost::program_options::value<vector<string>>()->multitoken()\
     ->zero_tokens()->composing(), "One or more entry points")
    ;

  boost::program_options::variables_map vm;

  try {
    boost::program_options::store(
                                boost::program_options::parse_command_line(argc, argv, options),
                                vm);
    boost::program_options::notify(vm);
  } catch (boost::program_options::error &err) {
    cerr << "could not parse the command line arguments" << endl;
    return -1;
  }


  if (vm.count("log")) {
    initializeLogger(true);
  } else {
    initializeLogger(false);
  }

  if (!vm.count("module")) {
    cerr << "At least one module file should be provided" << endl;
    return -1;
  }

  ProjectIRDB db(vm["module"].as<vector<string>>());

  set<string> entry_points;
  string _e_t;
  if (!vm.count("entry-points")) {
    // if none specified, make main as the entry point
    if (!db.getFunction("main")) {
      cerr << "main function was not found in the module" << endl;
      return -1;
    }

    entry_points.insert("main");
    _e_t = "main";
  } else {
    // add all the entry points
    auto eps = vm["entry-points"].as<vector<string>>();
    for (auto &ep:eps) {
      if (!db.getFunction(ep)) {
        cerr << "function " << ep << " was not found in the module" << endl;
        return -1;
      }
      entry_points.insert(ep);

      if (_e_t == "") {
        _e_t = ep;
      }
    }
  }

  LLVMTypeHierarchy type_hierarchy(db);
  LLVMPointsToSet points_to(db);
  LLVMBasedICFG icfg(db, CallGraphAnalysisType::CHA,
                     entry_points,
                     &type_hierarchy, &points_to);


  // ofstream ofs("icfg.dot");
  // icfg.printAsDot(ofs);
  // ofs.close();

  string log_func_name = "_ZN5blink25WebGLRenderingContextBase17SynthesizeGLErrorEjPKcS2_NS0_24ConsoleDisplayPreferenceE";
  GLAnalysisProblem p(&db, &type_hierarchy,
                      &icfg, &points_to, _e_t);
  p.set_log_func_name(log_func_name);

  IFDSSolver<LLVMAnalysisDomainDefault> solver(p);
  solver.solve();

  return 0;
}
