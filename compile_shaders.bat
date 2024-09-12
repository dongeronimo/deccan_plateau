@echo off
SET "folder=%1\build/shaders"

IF NOT EXIST "%folder%" (
    mkdir "%folder%"
)

glslc.exe %1\shaders\hello_shader.frag -o %1\build/shaders/hello_shader_frag.spv
glslc.exe %1\shaders\hello_shader.vert -o %1\build/shaders/hello_shader_vert.spv
glslc.exe %1\shaders\gpu_picker.frag -o %1\build/shaders/gpu_picker_frag.spv
glslc.exe %1\shaders\gpu_picker.vert -o %1\build/shaders/gpu_picker_vert.spv