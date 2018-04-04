#ifndef FIGURES_H
#define FIGURES_H

#include <vector>
#include "Vertex2D.h"
#include "Vertex3D.h"

const float TRIANGLE_SIZE = 0.5f;

const std::vector<Vertex2D> TRIANGLE_VERTEXES = {
    {{TRIANGLE_SIZE, -TRIANGLE_SIZE}, {1.0f, 0.0f}},
    {{TRIANGLE_SIZE, TRIANGLE_SIZE}, {1.0f, 1.0f}},
    {{-TRIANGLE_SIZE, TRIANGLE_SIZE}, {0.0f, 0.0f}}
};

const float CUBE_SIZE = 0.5f;

const std::vector<Vertex3D> CUBE_VERTEXES = {
    {{-CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{CUBE_SIZE, CUBE_SIZE, CUBE_SIZE}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-CUBE_SIZE, CUBE_SIZE, CUBE_SIZE}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    
    {{-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> CUBE_INDICES = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

const float QUAD_SIZE = 1.0f;

const std::vector<Vertex2D> QUAD_VERTEXES = {
    {{-QUAD_SIZE, -QUAD_SIZE}, {0.0f, 0.0f}},
    {{QUAD_SIZE, -QUAD_SIZE}, {1.0f, 0.0f}},
    {{QUAD_SIZE, QUAD_SIZE}, {1.0f, 1.0f}},
    {{-QUAD_SIZE, QUAD_SIZE}, {0.0f, 1.0f}},
};

const std::vector<uint16_t> QUAD_INDICES = {
    0, 1, 2, 2, 3, 0
};

#endif
