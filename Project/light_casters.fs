#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;

// Материал
uniform sampler2D material_diffuse;
uniform sampler2D material_specular;
uniform float material_shininess;

// Освещение
uniform int lightMode;

// Направленный свет
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight light;

// Точечный свет
uniform vec3 lightPos;

// Spotlight
uniform vec3 spotlightDir;

// Функции освещения
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(vec3 lightPos, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(vec3 lightPos, vec3 lightDir, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result;

    if (lightMode == 1)
        result = CalcDirLight(light, norm, viewDir);
    else if (lightMode == 2)
        result = CalcPointLight(lightPos, norm, FragPos, viewDir);
    else
        result = CalcSpotLight(lightPos, spotlightDir, norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0);
}

// ---------- Functions ----------

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);

    vec3 ambient = light.ambient * vec3(texture(material_diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material_diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material_specular, TexCoords));
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(vec3 lightPos, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightColor = vec3(1.0);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);

    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

    vec3 ambient = 0.05 * vec3(texture(material_diffuse, TexCoords));
    vec3 diffuse = 0.8 * diff * vec3(texture(material_diffuse, TexCoords));
    vec3 specular = 1.0 * spec * vec3(texture(material_specular, TexCoords));
    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLight(vec3 lightPos, vec3 lightDir, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightColor = vec3(1.0);
    vec3 dirToFrag = normalize(lightPos - fragPos);
    float theta = dot(dirToFrag, normalize(-lightDir));
    float epsilon = 0.9 - 0.85;
    float intensity = clamp((theta - 0.85) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, dirToFrag), 0.0);
    vec3 reflectDir = reflect(-dirToFrag, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);

    vec3 ambient = 0.05 * vec3(texture(material_diffuse, TexCoords));
    vec3 diffuse = 0.8 * diff * vec3(texture(material_diffuse, TexCoords));
    vec3 specular = 1.0 * spec * vec3(texture(material_specular, TexCoords));
    return (ambient + (diffuse + specular) * intensity);
}
