#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "llvm/IR/Argument.h"

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

static void read_apinames_from_file(const string path, set<string> &res) {
  ifstream ifs(path);
  string api_name;
  if (ifs.is_open()) {
    while (getline(ifs, api_name)) {
      res.insert(api_name);
    }
  } else {
    cout << "file not opened" << endl;
  }
}

static void parse_one_api(ProjectIRDB &db,
                          const string &api_name,
                          set<string> &res,
                          map<string, set<const llvm::Function *>> &apiname2funcs) {

  static string demangled_prefix = "blink::WebGLRenderingContextBase";
  auto functions = db.getAllFunctions();

  for (auto function: functions) {
    string func_name = function->getName().str();

    if (isMangled(func_name)) {
      func_name = cxxDemangle(func_name);
    }

    if (func_name.find(demangled_prefix) == 0 &&
        func_name.find(api_name) != string::npos) {
      if (function->isDeclaration()) {
        cout << function->getName().str() << " is a declaration, skipping" << endl;
        continue;
      }

      if (func_name.find("blink::ExecutionContext") != string::npos) {
        continue;
      }

      cout << "*** Handling " << func_name << endl;

      // Handle the first argument
      // if it is not of the correct type, we do not consider it
      // to be an API, instead, it might be a static class method
      if (function->arg_size() == 0) {
        continue;
      }
      llvm::Argument *first_arg = function->getArg(0);
      // llvm::outs() << "First argument:" << *first_arg->getType() << "\n";
      auto *arg_type = first_arg->getType();

      if (auto *pt = llvm::dyn_cast<llvm::PointerType>(arg_type)) {
        // handling argument
        // the first argument must be a pointer
        auto *et = pt->getElementType();
        string arg_type_name = et->getStructName().str();
        cout << "first argument type name: " << arg_type_name << endl;
        if (arg_type_name.find(demangled_prefix) == string::npos) {
          cout << function->getName().str() << " desn't seem to be a method, skipping" << endl;
          continue;
        }
      } else {
        cout << function->getName().str() << " desn't seem to be a method, skipping" << endl;
        continue;
      }

      res.insert(function->getName().str());
      apiname2funcs[api_name].insert(function);
    }
  }
}

static void parse_apis(ProjectIRDB &db,
                       set<string> &api_names,
                       set<string> &res,
                       map<string, set<const llvm::Function *>> &apiname2funcs) {

  for (const auto &api_name: api_names) {
    parse_one_api(db, api_name, res, apiname2funcs);
  }
}

static void test() {

}

int main(int argc, const char **argv) {

  boost::program_options::options_description options("Test program commandline options");
  options.add_options()
    ("log", "enable logging output")
    ("test", "do some testing")
    ("module,m", boost::program_options::value<vector<string>>()->multitoken()\
     ->zero_tokens()->composing()->notifier(&validateParamModule), "Path(s) to IR Module file(s)")
    ("apifile", boost::program_options::value<string>(), "Path(s) to API file")
    ("api", boost::program_options::value<string>(), "the name of one api to analyze")
    ("logtag", boost::program_options::value<string>(), "Set filter tag")
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

    if (vm.count("logtag")) {
      setLoggerFilterTag(vm["logtag"].as<string>());
    }

  } else {
    initializeLogger(false);
  }

  if (vm.count("test")) {
    test();
    return 0;
  }

  if (!vm.count("module")) {
    cerr << "At least one module file should be provided" << endl;
    return -1;
  }

  if(!vm.count("apifile") && !vm.count("api")) {
    cerr << "Please provide the apifile argument or an API name" << endl;
    return -1;
  }

  ProjectIRDB db(vm["module"].as<vector<string>>());

  set<string> apinames;
  if (vm.count("api")) {
    string api_name = vm["api"].as<string>();
    apinames.insert(api_name);
  } else {
    string apifile = vm["apifile"].as<string>();
    read_apinames_from_file(apifile, apinames);
  }


  set<string> entry_points;
  map<string, set<const llvm::Function *>> apiname2funcs;

  parse_apis(db, apinames, entry_points, apiname2funcs);

  LLVMTypeHierarchy type_hierarchy(db);

  // ofstream ofs("type_hierarchy.txt");
  // type_hierarchy.print(ofs);
  // ofs.close();

  LLVMPointsToSet points_to(db);
  LLVMBasedICFG icfg(db, CallGraphAnalysisType::CHA,
                     entry_points,
                     &type_hierarchy, &points_to);

  string log_func_name = "_ZN5blink25WebGLRenderingContextBase17SynthesizeGLErrorEjPKcS2_NS0_24ConsoleDisplayPreferenceE";

  for (auto api_funcname: entry_points) {
    cout << "##########################################" << endl;
    cout << "Parsing log messages of " << api_funcname << endl;
    GLAnalysisProblem p(&db, &type_hierarchy,
                        &icfg, &points_to, api_funcname);
    p.set_log_func_name(log_func_name);
    IFDSSolver<LLVMAnalysisDomainDefault> solver(p);
    solver.solve();
    cout << "##########################################" << endl;

  }

  return 0;
}
