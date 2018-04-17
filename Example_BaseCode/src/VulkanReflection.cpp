#include "VulkanReflection.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cmath>
#include <algorithm>

// SPIRV Cross
#include <spirv.hpp>
#include <spirv_cross.hpp>
#include <spirv_common.hpp>
#include <spirv_cfg.cpp>

// SPIRV Reflect
#include <spirv_reflect.h>

#include "Helpers.h"


std::string convertTypeToNameSPIRVCross(spirv_cross::SPIRType::BaseType type){
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
    //spirv_cross::SPIREntryPoint& entryPoint = compiler.get_entry_point("main", spv::ExecutionModel::ExecutionModelVertex);
    
    //const std::vector<spirv_cross::CombinedImageSampler>& samplers = compiler.get_combined_image_samplers();
    
    //const std::vector<spv::Capability>& capabilities = compiler.get_declared_capabilities();
    
    //const std::vector<std::string>& extentions = compiler.get_declared_extensions();
    
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
    //for (const spirv_cross::Resource& resource : resources.sampled_images) {
        //uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        //uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        //LOG("Info about images:\n");
    //}
    
    //spirv_cross::SPIRConstant& constant = compiler.get_constant(0);
}

void reflectShaderUsingSPIRVReflect(const std::vector<unsigned char>& data){
    SpvReflectResult result = SPV_REFLECT_RESULT_NOT_READY;
    
    // Создаем рефлектор
    SpvReflectShaderModule module;
    result = spvReflectCreateShaderModule(data.size(), data.data(), &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    
    // Перечислим список входных переменных
    {
        LOG("Input variables info\n");
        uint32_t variablesCount = 0;
        spvReflectEnumerateInputVariables(&module, &variablesCount, nullptr);
        std::vector<SpvReflectInterfaceVariable*> variables(variablesCount);
        spvReflectEnumerateInputVariables(&module, &variablesCount, variables.data());
        for (SpvReflectInterfaceVariable* variable: variables) {
            LOG("\tVariable name: %s\n", variable->name);
            LOG("\t\t- location: %d\n", variable->location);
            LOG("\t\t- format: %d\n", variable->format);
            LOG("\t\t- component size: %d bits\n", variable->numeric.scalar.width);
            LOG("\t\t- component count: %d\n", variable->numeric.vector.component_count);
        }
    }
    
    // Перечислим список юниформов
    {
        LOG("Descriptor sets info\n");
        uint32_t setsCount = 0;
        spvReflectEnumerateDescriptorSets(&module, &setsCount, nullptr);
        std::vector<SpvReflectDescriptorSet*> sets(setsCount);
        spvReflectEnumerateDescriptorSets(&module, &setsCount, sets.data());
        for (SpvReflectDescriptorSet* setInfo: sets) {
            LOG("\tSet at index: %d\n", setInfo->set);
            for (uint32_t i = 0; i < setInfo->binding_count; i++) {
                SpvReflectDescriptorBinding* binding = setInfo->bindings[i];
                LOG("\t\tBinding with name: %s\n", binding->name);
                LOG("\t\t\t- index: %d\n", binding->binding);
                LOG("\t\t\t- descriptor type: %d\n", binding->descriptor_type);
                LOG("\t\t\t- resource type: %d\n", binding->resource_type);
                LOG("\t\t\t- type description name: %s\n", binding->type_description->type_name);
                for (uint32_t j = 0; j < binding->type_description->member_count; j++) {
                    SpvReflectTypeDescription* member = &(binding->type_description->members[j]);
                    LOG("\t\t\tMember with name: %s\n", member->struct_member_name ? member->struct_member_name : "none");
                    LOG("\t\t\t\t- component size: %d bits\n", member->traits.numeric.scalar.width);
                    LOG("\t\t\t\t- components count: %d\n", member->traits.numeric.vector.component_count);
                    LOG("\t\t\t\t- matrix column count: %d\n", member->traits.numeric.matrix.column_count);
                    LOG("\t\t\t\t- matrix row count: %d\n", member->traits.numeric.matrix.row_count);
                    LOG("\t\t\t\t- matrix stride: %d\n", member->traits.numeric.matrix.stride);
                }
            }
        }
    }
    
    // Перечислим список push констант
    {
        LOG("Push constants info\n");
        uint32_t pushCount = 0;
        spvReflectEnumeratePushConstantBlocks(&module, &pushCount, nullptr);
        std::vector<SpvReflectBlockVariable*> pushConsts(pushCount);
        spvReflectEnumeratePushConstantBlocks(&module, &pushCount, pushConsts.data());
        for (SpvReflectBlockVariable* pushConstant: pushConsts) {
            LOG("\tPush constant: %s\n", pushConstant->name);
            LOG("\t\t- offset: %d\n", pushConstant->offset);
            LOG("\t\t- absolute offset: %d\n", pushConstant->absolute_offset);
            LOG("\t\t- size: %d\n", pushConstant->size);
            LOG("\t\t- padded size: %d\n", pushConstant->padded_size);
            LOG("\t\t- component size: %d bits\n", pushConstant->numeric.scalar.width);
            LOG("\t\t- components count: %d\n", pushConstant->numeric.vector.component_count);
            LOG("\t\t- matrix column count: %d\n", pushConstant->numeric.matrix.column_count);
            LOG("\t\t- matrix row count: %d\n", pushConstant->numeric.matrix.row_count);
            LOG("\t\t- matrix stride: %d\n", pushConstant->numeric.matrix.stride);
            for (uint32_t j = 0; j < pushConstant->member_count; j++) {
                SpvReflectBlockVariable* member = &(pushConstant->members[j]);
                LOG("\t\tMember with name: %s\n", member->name);
                LOG("\t\t\t- offset: %d\n", member->offset);
                LOG("\t\t\t- absolute offset: %d\n", member->absolute_offset);
                LOG("\t\t\t- size: %d\n", member->size);
                LOG("\t\t\t- padded size: %d\n", member->padded_size);
                LOG("\t\t\t- component size: %d bits\n", member->numeric.scalar.width);
                LOG("\t\t\t- components count: %d\n", member->numeric.vector.component_count);
                LOG("\t\t\t- matrix column count: %d\n", member->numeric.matrix.column_count);
                LOG("\t\t\t- matrix row count: %d\n", member->numeric.matrix.row_count);
                LOG("\t\t\t- matrix stride: %d\n", member->numeric.matrix.stride);
            }
        }
    }

    
    // Destroy the reflection data when no longer required.
    spvReflectDestroyShaderModule(&module);
}
