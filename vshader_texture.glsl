#version 130

uniform mat4 modelview;
uniform mat4 projection;
uniform vec4 eyeposition;

uniform vec4 light1_pos;
uniform vec4 light1_diffuse_product, light1_ambient_product, light1_specular_product;
uniform vec4 light2_pos;
uniform vec4 light2_diffuse_product, light2_ambient_product, light2_specular_product;
uniform float shininess;

in vec4 vPosition;
in vec4 vNormal;
in vec4 TexCoord;

out vec4 color;
out vec3 fE1;
out vec3 fE2;
out vec3 fN;
out vec3 fL1;
out vec3 fL2;
out vec4 TexCoordPass;

void
main()
{
    gl_Position = projection * modelview * vPosition;

    fN = vNormal.xyz;
    fE1 = -(modelview*vPosition).xyz;
    fE2 = (eyeposition - vPosition).xyz;

    if ( light1_pos.w != 0.0 ) fL1 = light1_pos.xyz - (modelview*vPosition).xyz;
    else fL1 = light1_pos.xyz;

    if ( light2_pos.w != 0.0 ) fL2 = light2_pos.xyz - vPosition.xyz;
    else fL2 = light2_pos.xyz;
    
    TexCoordPass = TexCoord;
}
