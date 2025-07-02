#version 450

layout(binding = 0) uniform MVP {
    mat4 modelView;
} mvp;

layout(location = 0) in vec3 inPosition;
//layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = mvp.modelView * vec4(inPosition, 1.0);
    fragColor = inPosition; //inColor;
}
