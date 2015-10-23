#version 130

uniform mat4 modelview;
uniform mat4 projection;

in vec4 vVertex;
in vec4 vColor;

out vec4 color;

void
main()
{
    gl_Position = projection * modelview * vVertex;
    color = vColor;
}
