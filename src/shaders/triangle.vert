#version 450

// triangle postions, in a real program these would be passed in as a vertex buffer
vec2 positions[3] = vec2[](
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(0.0, 0.5)
);

// triangle colors
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
); // votv reference?

// output color to the fragment shader, can output multiple colors to different locations, but we only have one color output in this case
layout(location = 0) out vec3 fragColor;

void main() {
    // set the position of the vertex
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    // set the color of the fragment
    fragColor = colors[gl_VertexIndex]; // gl_VertexIndex is a built-in variable that gives the index of the vertex being processed
}
