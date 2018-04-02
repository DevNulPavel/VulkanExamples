#ifndef FIGURES_H
#define FIGURES_H

#include <vector>
#include "Vertex3D.h"
#include "Vertex2D.h"

//const std::vector<Vertex2D> TRIANGLE_VERTEXES = {
//    {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
//    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
//    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
//};

const float QUAD_SIZE = 0.9f;

const std::vector<Vertex2D> QUAD_VERTEXES = {
    {{-QUAD_SIZE, -QUAD_SIZE}, {0.0f, 0.0f}},
    {{QUAD_SIZE, -QUAD_SIZE}, {1.0f, 0.0f}},
    {{QUAD_SIZE, QUAD_SIZE}, {1.0f, 1.0f}},
    {{-QUAD_SIZE, QUAD_SIZE}, {0.0f, 1.0f}},
    
    {{-QUAD_SIZE, -QUAD_SIZE}, {0.0f, 0.0f}},
    {{QUAD_SIZE, -QUAD_SIZE}, {1.0f, 0.0f}},
    {{QUAD_SIZE, QUAD_SIZE}, {1.0f, 1.0f}},
    {{-QUAD_SIZE, QUAD_SIZE}, {0.0f, 1.0f}}
};

const std::vector<uint32_t> QUAD_INDICES = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};


#endif
