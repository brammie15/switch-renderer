#version 320 es
precision mediump float;

in vec2 vtxTexCoord;
in vec4 vtxNormalQuat;
in vec3 vtxView;

out vec4 fragColor;

uniform vec4 lightPos;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec4 specular;

uniform sampler2D tex_diffuse;

vec3 quatrotate(vec4 q, vec3 v)
{
    vec3 t = 2.0 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

void main()
{
    vec3 normal = normalize(quatrotate(normalize(vtxNormalQuat), vec3(0.0, 0.0, 1.0)));
    vec3 fragPos = -vtxView;

    vec3 lightVec;
    float attenuation = 1.0;

    if (lightPos.w != 0.0)
    {
        vec3 lightDir = lightPos.xyz - fragPos;
        float dist = length(lightDir);
        lightVec = lightDir / dist;
        attenuation = 1.0 / (dist*dist + 0.1); // prevent extreme dark
    }
    else
    {
        lightVec = normalize(lightPos.xyz);
    }

    vec3 viewVec = normalize(-fragPos);
    vec3 halfVec = normalize(viewVec + lightVec);

    float diff = max(dot(normal, lightVec), 0.0);
    float spec = pow(max(dot(normal, halfVec), 0.0), specular.w);

    vec4 texColor = texture(tex_diffuse, vtxTexCoord);

    vec3 ambientColor = ambient * texColor.rgb * 1.5;
    vec3 diffuseColor = diffuse * diff * texColor.rgb * 1.2;
    vec3 specColor = specular.xyz * spec;

    vec3 color = ambientColor + attenuation * (diffuseColor + specColor);

    fragColor = vec4(pow(min(color, 1.0), vec3(1.0/2.2)), texColor.a); // gamma correction
}
