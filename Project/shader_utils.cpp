#include "shader_utils.hpp"
#include <stdexcept>

VkShaderModule createShaderModuleFromSPVdata(VkDevice device, const std::vector<uint32_t>& spv) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spv.size() * sizeof(uint32_t);
    createInfo.pCode = spv.data();

    VkShaderModule module;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }
    return module;
}
