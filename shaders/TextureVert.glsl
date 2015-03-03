#version 400

/// @brief MVP passed from app
uniform mat4 MVP;
/// @brief MV passed from app
uniform mat4 MV;
/// @brief our normal matrix
uniform mat4 normalMat;
// first attribute the vertex values from our VAO
layout (location=0) in vec3 inVert;
// first attribute the normal values from our VAO
layout (location=1) in vec3 baseNormal;
// second attribute the UV values from our VAO
layout (location=2) in vec2 inUV;
// we use this to pass the UV values to the frag shader
out vec2 vertUV;
// the position of our vertex multiplied by our MV
out vec3 position;
// pass the normal of our vertex
out vec3 normal;
// our normal matrix pass to frag
out mat4 normalMatrix;


void main()
{
    // pass our normal matrix the fragment shader
    normalMatrix = normalMat;
    //normalize normal just in case
    normal = normalize(baseNormal);
    // calculate the position
    position = vec3(MV * vec4(inVert,1.0));
    // pre-calculate for speed we will use this a lot

    // calculate the vertex position
    gl_Position = MVP*vec4(inVert, 1.0);
    // pass the UV values to the frag shader
    vertUV=inUV.st;
}
