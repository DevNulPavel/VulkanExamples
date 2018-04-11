#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D texTexture;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sampler2D(texTexture, texSampler), fragTexCoord);
    //outColor = vec4(fragTexCoord.x, fragTexCoord.y, 0.0, 0.5);
    //outColor = vec4(fragColor, 1.0);
    //outColor = texture(texSampler, vec2(0.5, 0.5));
}