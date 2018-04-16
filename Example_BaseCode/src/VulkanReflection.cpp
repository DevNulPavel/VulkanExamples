#include "VulkanReflection.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cmath>
#include <algorithm>

// SPIRV
#include <spirv.hpp>
#include <spirv_cross.hpp>
#include <spirv_common.hpp>
#include <spirv_cfg.cpp>

#include "Helpers.h"


std::string convertTypeToName(spirv_cross::SPIRType::BaseType type){
    std::string typeName;
    switch (type) {
        case spirv_cross::SPIRType::BaseType::Float:
            typeName = "Float";
            break;
        case spirv_cross::SPIRType::BaseType::Int:
            typeName = "Int";
            break;
        case spirv_cross::SPIRType::BaseType::Sampler:
            typeName = "Sampler";
            break;
        case spirv_cross::SPIRType::BaseType::SampledImage:
            typeName = "SamplerAndImage";
            break;
        case spirv_cross::SPIRType::BaseType::Image:
            typeName = "Image";
            break;
        case spirv_cross::SPIRType::BaseType::Struct:
            typeName = "Struct";
            break;
        default:
            typeName = "Unknown";
            break;
    }
    return typeName;
}

// Запускаем рефлексию шейдера
void reflectShaderUsingSPIRVCross(const std::vector<unsigned char>& data){    
    spirv_cross::Compiler compiler((uint32_t*)data.data(), data.size() / sizeof(uint32_t));
    spirv_cross::SPIREntryPoint& entryPoint = compiler.get_entry_point("main", spv::ExecutionModel::ExecutionModelVertex);
    
    const std::vector<spirv_cross::CombinedImageSampler>& samplers = compiler.get_combined_image_samplers();
    
    const std::vector<spv::Capability>& capabilities = compiler.get_declared_capabilities();
    
    const std::vector<std::string>& extentions = compiler.get_declared_extensions();
    
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    
    vector<spirv_cross::SpecializationConstant> constants = compiler.get_specialization_constants();
    
    vector<spirv_cross::EntryPoint> entryPoints = compiler.get_entry_points_and_stages();
    
    for (const spirv_cross::Resource& uniformBuffer: resources.uniform_buffers) {
        const std::string& bufferName = uniformBuffer.name;
        uint32_t bufferId = uniformBuffer.id;
        uint32_t typeId = uniformBuffer.type_id;
        uint32_t baseTypeId = uniformBuffer.base_type_id;
        
        uint32_t bindingIndex = compiler.get_decoration(bufferId, spv::DecorationBinding);
        uint32_t descriptoreSetIndex = compiler.get_decoration(bufferId, spv::DecorationDescriptorSet);
        uint32_t locationIndex = compiler.get_decoration(bufferId, spv::DecorationLocation);
        
        const spirv_cross::SPIRType& uniformBufferTypeInfo = compiler.get_type(typeId);
        const std::string& uniformBufferVariableName = compiler.get_name(bufferId);
        //size_t uniformSize = compiler.get_declared_struct_size(uniformBufferInfo);
        
        //const std::string& memberName = compiler.get_member_name(bufferId, 0);
        //size_t memberSize = compiler.get_declared_struct_member_size(uniformBufferInfo, 0);
        
        LOG("   Info about uniform buffer:\n");
        LOG("   -> buffer id: %d\n", bufferId);
        LOG("   -> buffer type id: %d\n", typeId);
        LOG("   -> buffer base type id: %d\n", baseTypeId);
        LOG("   -> buffer name: %s\n", bufferName.c_str());
        LOG("   -> variable name: %s\n", uniformBufferVariableName.c_str());
        //LOG("-> size: %d\n", (uint32_t)uniformSize);
        LOG("   -> binding: %d\n", bindingIndex);
        LOG("   -> set index: %d\n", descriptoreSetIndex);
        LOG("   -> location: %d\n", locationIndex);
        
        std::vector<spirv_cross::BufferRange> ranges = compiler.get_active_buffer_ranges(bufferId);
        for (const spirv_cross::BufferRange& rangeObj: ranges) {
            uint32_t index = rangeObj.index;
            size_t offset = rangeObj.offset;
            size_t range = rangeObj.range;
            
            const std::string& memberName = compiler.get_member_name(bufferId, index);
            const std::string& memberQualifiedName = compiler.get_member_qualified_name(bufferId, index);
            size_t memberSize = compiler.get_declared_struct_member_size(uniformBufferTypeInfo, index);
            
            LOG("   Varialbe with name: %s\n", memberName.c_str());
            LOG("       -> index: %d\n", index);
            LOG("       -> offset: %d\n", (int)offset);
            LOG("       -> range: %d\n", (int)range);
            LOG("       -> qualified name: %s\n", memberQualifiedName.c_str());
            LOG("       -> member size: %d\n", (int)memberSize);
        }
    }
    
    LOG("\n");
    
    // Push constants
    for (const spirv_cross::Resource& pushConst: resources.push_constant_buffers) {
        const std::string& constName = pushConst.name;
        uint32_t constId = pushConst.id;
        uint32_t typeId = pushConst.type_id;
        uint32_t baseTypeId = pushConst.base_type_id;
        
        uint32_t bindingIndex = compiler.get_decoration(constId, spv::DecorationBinding);
        uint32_t descriptoreSetIndex = compiler.get_decoration(constId, spv::DecorationDescriptorSet);
        uint32_t locationIndex = compiler.get_decoration(constId, spv::DecorationLocation);
        
        const std::string& pushConstantVariableName = compiler.get_name(constId);
        
        LOG("   Info about push constant:\n");
        LOG("   -> constant name: %s\n", constName.c_str());
        LOG("   -> constant id: %d\n", constId);
        LOG("   -> type id: %d\n", typeId);
        LOG("   -> base type id: %d\n", baseTypeId);
        LOG("   -> binding: %d\n", bindingIndex);
        LOG("   -> set index: %d\n", descriptoreSetIndex);
        LOG("   -> location: %d\n", locationIndex);
        LOG("   -> variable name: %s\n", pushConstantVariableName.c_str());
        
        const spirv_cross::SPIRType& pushConstInfoType = compiler.get_type(pushConst.type_id);
        
        std::vector<spirv_cross::BufferRange> ranges = compiler.get_active_buffer_ranges(constId);
        for (const spirv_cross::BufferRange& rangeObj: ranges) {
            uint32_t index = rangeObj.index;
            size_t offset = rangeObj.offset;
            size_t range = rangeObj.range;
            
            const std::string& memberName = compiler.get_member_name(constId, index);
            const std::string& memberQualifiedName = compiler.get_member_qualified_name(constId, index);
            size_t memberSize = compiler.get_declared_struct_member_size(pushConstInfoType, index);
            
            LOG("   Varialbe with name: %s\n", memberName.c_str());
            LOG("       -> index: %d\n", index);
            LOG("       -> offset: %d\n", (int)offset);
            LOG("       -> range: %d\n", (int)range);
            LOG("       -> qualified name: %s\n", memberQualifiedName.c_str());
            LOG("       -> member size: %d\n", (int)memberSize);
        }
        
        //const spirv_cross::SPIRType& pushConstInfoType = compiler.get_type(pushConst.type_id);
        //const spirv_cross::SPIRType& pushConstInfo = compiler.get_type_from_variable(pushConst.id);
        //const std::string& pushConstantName = compiler.get_name(pushConst.id);
        //size_t pushConstantSize = compiler.get_declared_struct_size(pushConstInfo);
        //std::string typeName = convertTypeToName(pushConstInfo.basetype);
    }
    
    // Images
    for (const spirv_cross::Resource& resource : resources.sampled_images) {
        uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        
        LOG("Info about images:\n");
    }
    
    //spirv_cross::SPIRConstant& constant = compiler.get_constant(0);
}
