@echo off
SET "folder=%1\build/shaders"

IF NOT EXIST "%folder%" (
    mkdir "%folder%"
)

glslc.exe %1\hello_shader.frag -o %1\build/shaders/hello_shader_frag.spv
glslc.exe %1\hello_shader.vert -o %1\build/shaders/hello_shader_vert.spv
glslc.exe %1\gpu_picker.frag -o %1\build/shaders/gpu_picker_frag.spv
glslc.exe %1\gpu_picker.vert -o %1\build/shaders/gpu_picker_vert.spv
glslc.exe %1\html.frag -o %1\build/shaders/html_frag.spv
glslc.exe %1\html.vert -o %1\build/shaders/html_vert.spv