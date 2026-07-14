#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0; // albedo; bound automatically, it's what DrawTexturePro draws
uniform sampler2D normalMap; // bound manuall via SetShaderValueTexture

#define MAX_LIGHTS 32
uniform vec3 lightDirs[MAX_LIGHTS]; // cam space, already rotated on CPU
uniform vec3 lightColors[MAX_LIGHTS]; // 0..1
uniform float lightIntensities[MAX_LIGHTS];
uniform int lightCount;
uniform float ambient;

out vec4 finalColor;

void main() {
    vec3 albedo = texture(texture0, fragTexCoord).rgb;
    vec3 n = normalize(texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0);

    vec3 lit = vec3(ambient);
    for (int i = 0; i < lightCount; i++) {
        float ndotl = max(0.0, dot(n, lightDirs[i]));
        lit += lightColors[i] * lightIntensities[i] * ndotl;
    }

    finalColor = vec4(albedo * lit, 1.0);
}
