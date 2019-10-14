#include <gtest/gtest.h>
#include <llvm/IR/InstIterator.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

class LLVMBasedBackwardICFGTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
#ifdef _WIN32
	  "build/x64-Clang-Debug/test/llvm_test_code/";
#else()
	  "build/test/llvm_test_code/";
#endif
};

TEST_F(LLVMBasedBackwardICFGTest, test1) {
  // TODO add suitable test cases
  // ASSERT_FALSE(true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}