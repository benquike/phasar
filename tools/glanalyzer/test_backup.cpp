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

namespace llvm {
class Value;
class Instruction;
class Type;
class Function;
class GlobalVariable;
} // namespace llvm

using namespace psr;
using namespace std;

/// MARK 0
#if 0
static bool check_file_arg(const char *path) {
  return boost::filesystem::exists(path) &&
         !boost::filesystem::is_directory(path);
}

static bool check_args(int argc, const char **argv) {

  if (argc < 3) {
    cerr << "Please provide LLVM IR file and API list file" << endl;
    return false;
  }

  if (!check_file_arg(argv[1])) {
    cerr << "Please provide the LLVM IR file as the first argument" << endl;
    return false;
  }

  if (!check_file_arg(argv[2])) {
    cerr << "Please provide the api list file as the second argument" << endl;
    return false;
  }

  return true;
}

void read_apinames_from_file(const char *path, set<string> &res) {
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

// find the first function definition containing the string specified in apiname
const llvm::Function *get_function_by_apiname(ProjectIRDB &db,
                                              string &apiname) {
  auto functions = db.getAllFunctions();
  const llvm::Function *ret = nullptr;
  for (auto it = functions.begin(); it != functions.end(); it++) {
    const llvm::Function *function = *it;
    string funcName = function->getName().str();
    if (isMangled(funcName)) {
      funcName = cxxDemangle(funcName);
    }

    if (funcName.find(apiname) != string::npos) {
      ret = function;
      break;
    }
  }

  return ret;
}

int main(int argc, const char **argv) {
  initializeLogger(false);
  if (!check_args(argc, argv)) {
    return -1;
  }

  // read the api names from a
  // file specified by argument
  set<string> api_names;
  read_apinames_from_file(argv[2], api_names);

  ProjectIRDB db({argv[1]});

  // get all the functions of each api
  //
  // we need some better logic to
  // get the function given an API
  auto functions = db.getAllFunctions();
  map<string, vector<const llvm::Function *>> name2funcs;
  for (const auto& api_name : api_names) {
    for (auto function : functions) {

      string funcName = function->getName().str();
      if (isMangled(funcName)) {
	funcName = cxxDemangle(funcName);
      }
      
      if (funcName.find("blink::WebGLRenderingContext") == 0 &&
	  funcName.find(api_name) != string::npos) {

	if (function->isDeclaration()) {
	  cout << function->getName().str() << " is a declaration" << endl;
	}

	name2funcs[api_name].push_back(function);
      }
    }
  }

  /*
  for (auto it : name2funcs) {
    cout << "+--" << it.first << endl;
    for (auto func : it.second) {
      cout << " |-- " << cxxDemangle(func->getName().str()) << endl;
    }
  }
  */


  set<string> entry_points;
  auto createShader_funcs = name2funcs.find("createShader");
  if (createShader_funcs != name2funcs.end()) {
    for (auto f:createShader_funcs->second) {
      entry_points.insert(f->getName().str());
    }
  }
  
  // extracting all log messages
  LLVMTypeHierarchy type_hierarchy(db);
  LLVMPointsToSet points_to(db);
  LLVMBasedICFG icfg(db, CallGraphAnalysisType::CHA, entry_points, &type_hierarchy, &points_to);

  for (auto func:createShader_funcs->second) {
    auto callsites = icfg.getCallsFromWithin(func);

    for (auto inst:callsites) {
      // if (icfg.isDirectCall(inst)) {
      // }

      auto callees = icfg.getCalleesOfCallAt(inst);
      cout << "size of callees is " << callees.size() << endl;
      for (auto callee: callees) {
	cout << "  |-- " << callee->getName().str() << endl;
      }
    }
  }
  
  return 0;
}

#endif
