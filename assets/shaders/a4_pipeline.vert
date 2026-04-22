#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;
out float vViewDepth;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    // TODO: Transform each vertex from object space to clip space.
    // Use the lecture equation: p_clip = P * V * M * p_obj
    // Also pass a view-space depth quantity to the fragment shader.
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
    vViewDepth = 1.0;
}
