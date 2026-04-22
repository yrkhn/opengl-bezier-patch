#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

// Создать VkShaderModule из массива uint32_t (SPIR-V)
VkShaderModule createShaderModuleFromSPVdata(VkDevice device, const std::vector<uint32_t>& spv);
