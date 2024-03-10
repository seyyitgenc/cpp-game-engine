#version 460 core

// struct Material {
//     sampler2D diffuse;
//     sampler2D specular;
//     float shininess;
// };

struct Light {
    vec3 color;
    vec3 position;
    float constant;
    float linear;
    float quadratic;
};

out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 FragPos;    

uniform vec3 viewPos;
uniform Light light;
// uniform Material material;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform float shininess;

void main() {
    // ambient
    vec3 ambient = 0.2 * light.color * vec3(texture(texture_diffuse1, TexCoords));

    float distance = length(light.position - FragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = light.color * diff * vec3(texture(texture_diffuse1, TexCoords));

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    vec3 specular = spec * vec3(texture(texture_specular1, TexCoords));

    vec3 result = (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0f);
}