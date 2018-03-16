#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

// GLM
#include <glm/glm.hpp>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

#endif
