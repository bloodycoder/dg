#ifndef DG_LLVM_INTERPROC_CD_H_
#define DG_LLVM_INTERPROC_CD_H_

#include "dg/llvm/ControlDependence/LLVMControlDependenceAnalysisImpl.h"

#include <set>
#include <map>
#include <unordered_map>


namespace llvm {
class Function;
}

namespace dg {

class LLVMPointerAnalysis;

namespace llvmdg {


class LLVMInterprocCD : public LLVMControlDependenceAnalysisImpl {
    LLVMPointerAnalysis *PTA{nullptr};

    struct FuncInfo {
        // points that may abort the program
        // (or cause infinite looping). That is,
        // points due to which the function may not return
        // to its caller
        std::set<const llvm::Value *> noret;
    };

    std::unordered_map<const llvm::Instruction *, std::set<llvm::Value *>> _instrCD;
    std::unordered_map<const llvm::BasicBlock *, std::set<llvm::Value *>> _blockCD;
    std::unordered_map<const llvm::Function *, FuncInfo> _funcInfos;

    FuncInfo *getFuncInfo(const llvm::Function *F) {
        auto it = _funcInfos.find(F);
        return it == _funcInfos.end() ? nullptr : &it->second;
    }

    bool hasFuncInfo(const llvm::Function *fun) const {
       return _funcInfos.find(fun) != _funcInfos.end();
    }

    void computeFuncInfo(const llvm::Function *fun);

    std::vector<const llvm::Function *> getCalledFunctions(const llvm::Value *v);

public:
    using ValVec = LLVMControlDependenceAnalysisImpl::ValVec;

    LLVMInterprocCD(const llvm::Module *module,
                    const LLVMControlDependenceAnalysisOptions& opts = {},
                    LLVMPointerAnalysis *pta = nullptr)
        : LLVMControlDependenceAnalysisImpl(module, opts), PTA(pta) {}

    /// Getters of dependencies for a value
    ValVec getDependencies(const llvm::Instruction *I) override {
        auto *fun = I->getParent()->getParent();
        if (!getFuncInfo(fun)) {
            computeFuncInfo(fun);
        }

        ValVec ret;
        auto instrIt = _instrCD.find(I);
        if (instrIt != _instrCD.end()) {
            ret.insert(ret.end(), instrIt->second.begin(), instrIt->second.end());
        }

        auto blkIt = _blockCD.find(I->getParent());
        if (blkIt != _blockCD.end()) {
            ret.insert(ret.end(), blkIt->second.begin(), blkIt->second.end());
        }

        return ret;
    }

    ValVec getDependent(const llvm::Instruction *) override { return {}; }

    /// Getters of dependencies for a basic block
    ValVec getDependencies(const llvm::BasicBlock *b) override { return {}; }
    ValVec getDependent(const llvm::BasicBlock *) override { return {}; }

    void run() { /* we run on demand */ }
};

} // namespace llvmdg
} // namespace dg

#endif
