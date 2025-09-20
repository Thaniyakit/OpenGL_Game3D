#version 330 core
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 viewPos;
uniform Material material;
uniform PointLight pointLights[1];

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(pointLights[0].position - FragPos);

    // Diffuse shading
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseTex = texture(material.diffuse, TexCoords).rgb;
    vec3 diffuse = pointLights[0].diffuse * diff * diffuseTex;

    // Ambient
    vec3 ambient = pointLights[0].ambient * diffuseTex;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specularTex = texture(material.specular, TexCoords).rgb;
    vec3 specular = pointLights[0].specular * spec * specularTex;

    // Attenuation
    float distance = length(pointLights[0].position - FragPos);
    float attenuation = 1.0 / (pointLights[0].constant + pointLights[0].linear * distance +
                               pointLights[0].quadratic * (distance * distance));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
