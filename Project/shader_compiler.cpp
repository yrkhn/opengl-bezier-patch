#include "shader_compiler.hpp"
#include <shaderc/shaderc.hpp>
#include <stdexcept>

std::vector<uint32_t> compileGLSLtoSPV(const std::string& source, shaderc_shader_kind kind, const std::string& source_name) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string err = module.GetErrorMessage();
        throw std::runtime_error("Shader compilation failed (" + source_name + "):\n" + err);
    }

    // копируем в vector<uint32_t>
    return std::vector<uint32_t>(module.cbegin(), module.cend());
}
