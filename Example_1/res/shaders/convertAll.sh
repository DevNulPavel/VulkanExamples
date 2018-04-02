#! /usr/bin/env bash

glslangValidator -V model_shader.frag -o model_shader_frag.spv
glslangValidator -V model_shader.vert -o model_shader_vert.spv

glslangValidator -V post_shader.frag -o post_shader_frag.spv
glslangValidator -V post_shader.vert -o post_shader_vert.spv