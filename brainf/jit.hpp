#pragma once

#include "llvm.hpp"

#include <memory>

namespace bf {
class jit {
  // TODO: properly treat llvm::Error/llvm::Expected
  llvm::orc::ExecutionSession m_es{
      llvm::cantFail(llvm::orc::SelfExecutorProcessControl::Create())};
  llvm::orc::JITTargetMachineBuilder m_jtmb{
      m_es.getExecutorProcessControl().getTargetTriple()};
  llvm::DataLayout m_dl{llvm::cantFail(m_jtmb.getDefaultDataLayoutForTarget())};

  llvm::orc::MangleAndInterner m_mangle{m_es, m_dl};

  llvm::orc::RTDyldObjectLinkingLayer m_object_layer{
      m_es, [] { return std::make_unique<llvm::SectionMemoryManager>(); }};
  llvm::orc::IRCompileLayer m_compile_layer{
      m_es, m_object_layer,
      std::make_unique<llvm::orc::ConcurrentIRCompiler>(m_jtmb)};

  llvm::orc::JITDylib &m_jd{m_es.createBareJITDylib("<main>")};

public:
  jit() {
    m_jd.addGenerator(llvm::cantFail(
        llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            m_dl.getGlobalPrefix())));
  }
  ~jit() {
    if (auto err = m_es.endSession()) {
      m_es.reportError(std::move(err));
    }
  }

  void add_module(llvm::orc::ThreadSafeModule tsm) {
    tsm.withModuleDo([this](llvm::Module &m) { m.setDataLayout(m_dl); });
    // TODO: Use a ResourceTracker to track memory resources used by JIT
    llvm::cantFail(m_compile_layer.add(m_jd, std::move(tsm)));
  }

  [[nodiscard]] auto lookup(llvm::StringRef name) {
    return m_es.lookup({&m_jd}, m_mangle(name.str()));
  }
};
} // namespace bf
