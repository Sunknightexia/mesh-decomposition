#version 330 core
out vec4 FragColor;
out vec4 color;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;

//光照
struct Light {
    vec3 position;
    vec3 ambient;//环境光照
    vec3 diffuse;//漫反射光照
    vec3 specular;//镜面反射光照
};
uniform Light light;


void main()
{
    //环境光ambient
    //环境颜色 = 光源颜色 × 环境光照强度 × 贴图
    vec3 ambient = vec3(1.0f,1.0f,1.0f) * light.ambient ;

    //漫反射diffuse
    //DiffuseFactor = max(0, dot(N, L))
    //漫反射颜色 = 光源颜色 × 漫反射因子(diffuseFactor) × 漫反射光照强度 × 贴图
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(0.0f,0.0f,3.0f) - FragPos);
    float diffuseFactor = max(dot(norm, lightDir),0.0);
    vec3 diffuse = vec3(1.0f,1.0f,1.0f) * diffuseFactor * light.diffuse ;

    //镜面反射specular
    //R=reflect(L, N)
    //SpecularFactor = pow(max(dot(R,V),0.0), shininess)
    //镜面反射颜色 = 光源颜色 × 镜面反射因子(SpecularFactor) × 镜面光照强度 × 贴图
    vec3 viewDir = normalize(vec3(0.0f,0.0f,3.0f) - FragPos);
    vec3 reflectDir = reflect(-lightDir , norm);
    float specularFactor = pow(max(dot(viewDir, reflectDir),0.0),64.0f);
    vec3 specular = vec3(1.0f,1.0f,1.0f) * specularFactor * light.specular ;

    //最终片段颜色：环境颜色+漫反射颜色+镜面反射颜色
    vec3 result = ambient + diffuse + specular;
    color = vec4(result , 1.0f);
}
