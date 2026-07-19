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

// skybox (equirectangular panorama), sampled where no geometry was drawn
uniform sampler2D skyMap;
uniform int useSky;
uniform vec3 camRight;   // camera basis in world space
uniform vec3 camUp;
uniform vec3 camFwd;
uniform float focalLen;
uniform float aspectRatio;

// speed / boost screen FX
uniform float boostAmount; // eased 0..1, boost active
uniform float speedAmount; // 0..1 fraction of top speed
uniform float time;

out vec4 finalColor;

void main() {
    vec4 albedoSample = texture(texture0, fragTexCoord);
    vec2 ndc = vec2(fragTexCoord.x * 2.0 - 1.0,
                    1.0 - fragTexCoord.y * 2.0);
    vec3 col;

    // alpha 0 = background sentinel (albedo buffer cleared to it, geometry
    // always writes alpha 255): reconstruct the world-space view ray and
    // sample the sky panorama, skipping lighting entirely
    if (useSky == 1 && albedoSample.a < 0.5) {
        // inverse of project(): x/z = ndc.x/focal, y/z = ndc.y/(focal*aspect)
        vec3 dir = normalize(camFwd
                           + camRight * (ndc.x / focalLen)
                           + camUp * (ndc.y / (focalLen * aspectRatio)));
        float u = atan(dir.x, dir.z) * 0.15915494 + 0.5;  // 1/(2*pi)
        float v = 0.5 - asin(clamp(dir.y, -1.0, 1.0)) * 0.31830989; // 1/pi
        col = texture(skyMap, vec2(u, v)).rgb;
    } else {
        vec3 n = normalize(texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0);

        vec3 lit = vec3(ambient);
        for (int i = 0; i < lightCount; i++) {
            float ndotl = max(0.0, dot(n, lightDirs[i]));
            lit += lightColors[i] * lightIntensities[i] * ndotl;
        }
        col = albedoSample.rgb * lit;
    }

    // speed lines: fixed thin spokes with dashes racing OUTWARD from the
    // screen center (radial motion = forward speed, no rotation). Fade in
    // near top speed, full strength while boosting.
    float lineStrength = max(boostAmount,
                             smoothstep(0.8, 1.0, speedAmount) * 0.4);
    if (lineStrength > 0.001) {
        float r = length(ndc);
        float ang = atan(ndc.y, ndc.x);

        // thin static spokes around the screen
        float spokes = sin(ang * 26.0) * 0.5 + 0.5;
        spokes = pow(spokes, 18.0);

        // dashes travel from center to edge; tail fades behind the head
        float flow = fract(r * 2.5 - time * 3.5);
        flow *= flow;

        // edges only, gentle brightness
        float streak = spokes * flow * smoothstep(0.65, 1.1, r);
        col += vec3(streak * 0.12 * lineStrength);
    }

    // boost-only grade: neutral (colorless) edge vignette, no tint
    if (boostAmount > 0.001) {
        float r = length(ndc);
        col *= 1.0 - 0.12 * boostAmount * smoothstep(0.8, 1.5, r);
    }

    finalColor = vec4(col, 1.0);
}
