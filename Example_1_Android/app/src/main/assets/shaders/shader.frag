#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler2D texSampler;

// Push const
layout(push_constant) uniform PushConstantsFrag {
	layout(offset = 64) vec4 color;
} pcFrag;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord) * pcFrag.color;
}