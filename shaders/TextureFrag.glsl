#version 400

struct LightInfo
{
    // Light position in eye coords.
    vec3 position;
    // Ambient light intensity
    vec3 La;
    // Diffuse light intensity
    vec3 Ld;
    // Specular light intensity
    vec3 Ls;
};
uniform LightInfo light;

struct MaterialInfo
{
        // Ambient reflectivity
        vec3 Ka;
        // Diffuse reflectivity
        vec3 Kd;
        // Specular reflectivity
        vec3 Ks;
        // Specular shininess factor
        float shininess;
};
uniform MaterialInfo material;

// this is a pointer to the current 2D texture object
uniform sampler2D baseTex;
// this is a pointer to our rust location data
uniform sampler2D rustLoc;
// our DPD model lattice texture
uniform sampler2D latticeTex;
// a bool so we know weather to draw our lattice texture or not
uniform bool drawLattice;
// The start colour of our rust
uniform vec4 rustStartColour;
// The end colour of our rust
uniform vec4 rustEndColour;
// the vertex UV
smooth in vec2 vertUV;
//our vertex postion
smooth in vec3 position;
// our normals of our mesh
smooth in vec3 normal;
// our normal matrix
in mat4 normalMatrix;
// the final fragment colour
out vec4 outColour;
vec3 spec;
vec4 ambAndDiff;

void phongModel()
{
        vec4 s = vec4(normalize(light.position - position),0.0);
        vec4 v = vec4(normalize(-position),0.0);
        vec4 r = reflect( -s, normalize(normalMatrix * vec4(normal, 0.0) ));
        vec4 ambient = vec4(light.La * material.Ka,0.0);
        float sDotN = max( dot(s,vec4(normal,0.0)), 0.0 );
        vec4 diffuse = vec4(light.Ld * material.Kd * sDotN,0.0);
        ambAndDiff = ambient + diffuse;
        if( sDotN > 0.0 )
        {
                spec = light.Ls * material.Ks * pow( max( dot(r,v), 0.0 ), material.shininess );
        }
}

void main ()
{
    vec4 colour;
    float rustSize = texture(rustLoc, vertUV).b;
    vec4 rustColour = mix(rustStartColour, rustEndColour, rustSize);
    if(drawLattice){
        float latticeValue = texture(latticeTex,vertUV).r;
        colour = mix(vec4(latticeValue,latticeValue,latticeValue,1.0),rustColour,rustSize);
    }
    else{
        spec = vec3(0);
        ambAndDiff = vec4(0);
        phongModel();
        vec4 texColour = texture(baseTex,vertUV);
        texColour = mix(texColour,rustColour,rustSize);
        float shine = 1-rustSize;
//        colour = ambAndDiff * texColour + vec4(spec,shine);
        colour = texColour;
    }


//    if(rustSize>0){
//        discard;
//    }
//    if (rustSize >= 0.98){
//        discard;
//    }
//    else{
        outColour = colour;
//    }

}
