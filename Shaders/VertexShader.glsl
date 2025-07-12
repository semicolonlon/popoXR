#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
uniform mat4 modelViewProjection;
uniform float pointSize;
out vec3 fragColor;
void main() {
    fragColor = vertexColor;
    gl_Position = modelViewProjection * vec4(vertexPosition, 1.0);
    gl_PointSize = pointSize;
}