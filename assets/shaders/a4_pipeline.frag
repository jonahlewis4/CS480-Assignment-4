#version 330 core
in vec3 vColor;
in float vViewDepth;
out vec4 FragColor;

uniform bool uDepthView;
uniform float uNearPlane;
uniform float uFarPlane;

void main()
{
    // TODO: Output the interpolated color from the vertex shader.
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
