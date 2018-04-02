#version 450
#extension GL_ARB_separate_shader_objects : enable

// Varying
layout(location = 0) in vec2 fragTexCoord;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
	float power;
} ubo;
layout(binding = 1) uniform sampler2D texSampler;

// Out
layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(texSampler, fragTexCoord);
    float midValue = (color.r + color.g + color.b) / 3.0;

    outColor = vec4(mix(color.rgb, vec3(midValue, midValue*0.6, midValue*0.6), ubo.power), color.a);

    //outColor = vec4(1.0, 1.0, 1.0, 1.0);
}