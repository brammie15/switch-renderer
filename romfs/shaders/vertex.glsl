#version 320 es
precision mediump float;

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

// Outputs to fragment shader
out vec2 vTexCoord;
out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBitangent;

// Uniforms
uniform mat4 uModel; // model matrix
uniform mat4 uView;  // view matrix
uniform mat4 uProj;  // projection matrix

void main()
{
    // World-space position
    vec4 worldPos = uModel * vec4(inPosition, 1.0);
    vWorldPos = worldPos.xyz;

    // Transform normal and tangent/bitangent to world space
    mat3 normalMatrix = mat3(uModel); // assumes no non-uniform scale
    vNormal = normalize(normalMatrix * inNormal);
    vTangent = normalize(normalMatrix * inTangent);
    vBitangent = normalize(normalMatrix * inBitangent);

    // Pass UVs
    vTexCoord = inTexCoord;

    // Clip-space position
    gl_Position = uProj * uView * worldPos;
}
