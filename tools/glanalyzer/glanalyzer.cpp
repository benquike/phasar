/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <fstream>
#include <iostream>
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

int main(int Argc, const char **Argv) {
  initializeLogger(false);
  if (Argc < 2 || !boost::filesystem::exists(Argv[1]) ||
      boost::filesystem::is_directory(Argv[1])) {
    std::cerr << "glanalyzer\n"
                 "A small PhASAR-based analyzer on webgl interface code\n\n"
                 "Usage: glanalyzer <LLVM IR file>\n";
    return 1;
  }
  ProjectIRDB DB({Argv[1]});

  /*
    auto functions = DB.getAllFunctions();
    llvm::Function const *target = nullptr;

    for (auto it = functions.begin(); it != functions.end(); it ++) {
      string funcName = (*it)->getName().str();
      if (isMangled(funcName)) {
        funcName = cxxDemangle(funcName);
      }
      if (funcName.find("blink::WebGLRenderingContext") == 0 &&
    funcName.find("createShader") != std::string::npos) { target = *it;
      }
    }
    if (target == nullptr) {
      cout << "Target function not found" << endl;
    } else {
      cout << "Target function found" << endl;
    }
  */

  LLVMTypeHierarchy H(DB);
  // print type hierarchy
  // H.print();

  /*
    auto types = H.getAllTypes();
    for (auto it = types.begin(); it != types.end(); it++) {
      cout << (*it)->getName().str() << endl;
    }
  */

  // LLVMTypeHierarchy H(DB);
  // H.print();

  const llvm::StructType *WebGLRenderingBaseType =
      H.getType("class.blink::WebGLRenderingContextBase");

  if (WebGLRenderingBaseType) {
    cout << "WebGLRenderingBaseType found" << endl;
    if (H.hasVFTable(WebGLRenderingBaseType)) {
      cout << "Virtual function table found" << endl;
    } else {
      cout << "Virtual function table not found" << endl;
    }

  } else {
    cout << "WebGLRenderingBaseType not found" << endl;
  }

  return 0;
}
