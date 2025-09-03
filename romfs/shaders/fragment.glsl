#version 320 es
precision mediump float;

in vec2 vTexCoord;
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;

out vec4 fragColor;

uniform vec3 uCamPos;
uniform vec3 uLightDir;   // directional light, should be normalized
uniform vec3 uLightColor;

uniform sampler2D texBaseColor;
uniform sampler2D texMetallic;
uniform sampler2D texRoughness;
uniform sampler2D texAO;
uniform sampler2D texNormal;

const float PI = 3.14159265359;

vec3 getNormal()
{
    // vec3 n = texture(texNormal, vTexCoord).xyz * 2.0 - 1.0; // normal map
    // mat3 TBN = mat3(normalize(vTangent), normalize(vBitangent), normalize(vNormal));
    // return normalize(TBN * n);

    return normalize(vNormal);
}

// Fresnel Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX normal distribution
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N,H),0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2*(a2-1.0)+1.0);
    return a2 / (PI * denom * denom);
}

// Geometry (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r)/8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N,V),0.0);
    float NdotL = max(dot(N,L),0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

void main()
{
    vec3 albedo = pow(texture(texBaseColor, vTexCoord).rgb, vec3(2.2)); // sRGB -> linear
    float metallic = texture(texMetallic, vTexCoord).r;
    float roughness = texture(texRoughness, vTexCoord).r;
    float ao = texture(texAO, vTexCoord).r;
    // float ao = 1.0;


    vec3 N = getNormal();
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 L = normalize(-uLightDir);  // directional light points *to* the surface
    vec3 H = normalize(V + L);

    // PBR calculations
    float NDF = distributionGGX(N,H,roughness);
    float G = geometrySmith(N,V,L,roughness);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H,V),0.0), F0);

    float NdotL = max(dot(N,L),0.0);
    float NdotV = max(dot(N,V),0.0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = nominator / denominator;

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    vec3 radiance = uLightColor;
    vec3 color = (diffuse + specular) * radiance * NdotL;
    // color *= ao; // apply AO
    color = pow(color, vec3(1.0/2.2)); // gamma correction

    fragColor = vec4(color, 1.0);

    // fragColor = vec4(texture(texNormal, vTexCoord).xyz, 1.0);

    
}
