#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

#endif
