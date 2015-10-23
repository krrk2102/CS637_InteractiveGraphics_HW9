#version 130

uniform mat4 modelview;

uniform vec4 light1_diffuse_product, light1_ambient_product, light1_specular_product;
uniform vec4 light2_diffuse_product, light2_ambient_product, light2_specular_product;
uniform float shininess;
uniform sampler2D texture;

in vec4 color;
in vec3 fE1;
in vec3 fE2;
in vec3 fN;
in vec3 fL1;
in vec3 fL2;
in vec4 TexCoordPass;

void
main()
{
    vec4 textureColor = texture2D( texture, TexCoordPass.st );
    
    vec3 diffuse1, diffuse2, specular1, specular2;

    vec3 E1 = normalize( fE1 );
    vec3 E2 = normalize( fE2 );
    vec3 N1 = normalize( ( modelview*vec4(fN, 0) ).xyz );
    vec3 N2 = normalize( fN );
    vec3 L1 = normalize( fL1 );
    vec3 L2 = normalize( fL2 );
    vec3 H1 = normalize( L1 + E1 );
    vec3 H2 = normalize( L2 + E2 );

    float Kd1 = max( dot( L1, N1 ), 0 );
    float Kd2 = max( dot( L2, N2 ), 0 );
    diffuse1 = (Kd1 * (0.3 * textureColor + 0.5 * light1_diffuse_product)).xyz;
    diffuse2 = (Kd2 * (0.2 * textureColor + 0.4 * light2_diffuse_product)).xyz;

    float Ks1 = pow( max( dot( N1, H1 ), 0 ), shininess );
    float Ks2 = pow( max( dot( N2, H2 ), 0 ), shininess );
    specular1 = (Ks1 * light1_specular_product).xyz;
    specular2 = (Ks2 * light2_specular_product).xyz;
    if ( dot( L1, N1 ) < 0.0 ) specular1 = vec3( 0.0, 0.0, 0.0 );
    if ( dot( L2, N2 ) < 0.0 ) specular2 = vec3( 0.0, 0.0, 0.0 );

    gl_FragColor = vec4( diffuse1+diffuse2+specular1+specular2+light1_ambient_product.xyz+light2_ambient_product.xyz, 1 );
}
