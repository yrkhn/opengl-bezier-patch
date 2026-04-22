#pragma once

#include <vector>
#include <string>
#include <cstdint>

// shaderc forward declare (не нужен в хэдере, но удобно)
#include <shaderc/shaderc.hpp>

// Компиляция GLSL-строки в SPIR-V (вектор uint32_t)
std::vector<uint32_t> compileGLSLtoSPV(const std::string& source, shaderc_shader_kind kind, const std::string& source_name);
