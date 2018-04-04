#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;

// Uniforms
layout(binding = 0) uniform sampler2D texSampler;

// Push const
layout(push_constant) uniform PushConsts {
	float coeff;
} pushConsts;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord)*vec4(pushConsts.coeff, 0.8, 0.9, 1.0);
    //outColor = vec4(fragTexCoord.x, fragTexCoord.y, 0.0, 0.5);
    //outColor = vec4(fragColor, 1.0);
    //outColor = texture(texSampler, vec2(0.5, 0.5));
}