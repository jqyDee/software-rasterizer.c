#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0; // albedo; bound automatically, it's what DrawTexturePro draws
uniform sampler2D normalMap; // bound manuall via SetShaderValueTexture

#define MAX_LIGHTS 32
// must match MAX_KART_COUNT in src/rasterizer_module/game/kart.h
#define MAX_VIEWPORTS 4

uniform vec3 lightColors[MAX_LIGHTS]; // 0..1
uniform float lightIntensities[MAX_LIGHTS];
uniform int lightCount;
uniform float ambient;

// split-screen: one full-window draw call shades every viewport at once —
// each pixel looks up which viewport rect it falls in (render-buffer pixel
// space) and uses that viewport's own camera-space light dirs / cam basis /
// FX amounts, since normals were baked into camera space per-viewport on
// the CPU rasterization pass already
uniform vec4 viewportRects[MAX_VIEWPORTS]; // x, y, w, h in render-buffer px
uniform int viewportCount;
uniform vec2 bufferSize; // render-buffer dims, to map fragTexCoord -> px

// cam space, per-viewport — flattened (GLSL doesn't support 2D array
// uniforms); row-major, index with [vp * MAX_LIGHTS + lightId]
uniform vec3 lightDirs[MAX_VIEWPORTS * MAX_LIGHTS];

// skybox (equirectangular panorama), sampled where no geometry was drawn
uniform sampler2D skyMap;
uniform int useSky;
uniform vec3 camRight[MAX_VIEWPORTS]; // camera basis in world space
uniform vec3 camUp[MAX_VIEWPORTS];
uniform vec3 camFwd[MAX_VIEWPORTS];
uniform float focalLen[MAX_VIEWPORTS];
uniform float aspectRatio[MAX_VIEWPORTS];

// speed / boost screen FX
uniform float boostAmount[MAX_VIEWPORTS]; // eased 0..1, boost active
uniform float speedAmount[MAX_VIEWPORTS]; // 0..1 fraction of top speed
uniform float time;

out vec4 finalColor;

void main() {
    vec4 albedoSample = texture(texture0, fragTexCoord);

    // which viewport is this pixel in? render-buffer pixel space, same
    // rects compute_kart_viewport() produced on the CPU side
    vec2 pixelPos = fragTexCoord * bufferSize;
    int vp = 0;
    for (int i = 0; i < viewportCount; i++) {
        vec4 r = viewportRects[i];
        if (pixelPos.x >= r.x && pixelPos.x < r.x + r.z &&
            pixelPos.y >= r.y && pixelPos.y < r.y + r.w) {
            vp = i;
            break;
        }
    }
    vec4 vpRect = viewportRects[vp];

    // NDC relative to THIS viewport, not the whole window — the projection
    // that produced this pixel's geometry only spans this viewport's rect
    vec2 local = (pixelPos - vpRect.xy) / vpRect.zw; // 0..1 within the viewport
    vec2 ndc = vec2(local.x * 2.0 - 1.0, 1.0 - local.y * 2.0);
    vec3 col;

    // alpha 0 = background sentinel (albedo buffer cleared to it, geometry
    // always writes alpha 255): reconstruct the world-space view ray and
    // sample the sky panorama, skipping lighting entirely
    if (useSky == 1 && albedoSample.a < 0.5) {
        // inverse of project(): x/z = ndc.x/focal, y/z = ndc.y/(focal*aspect)
        vec3 dir = normalize(camFwd[vp]
                           + camRight[vp] * (ndc.x / focalLen[vp])
                           + camUp[vp] * (ndc.y / (focalLen[vp] * aspectRatio[vp])));
        float u = atan(dir.x, dir.z) * 0.15915494 + 0.5;  // 1/(2*pi)
        float v = 0.5 - asin(clamp(dir.y, -1.0, 1.0)) * 0.31830989; // 1/pi
        col = texture(skyMap, vec2(u, v)).rgb;
    } else {
        vec3 n = normalize(texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0);

        vec3 lit = vec3(ambient);
        for (int i = 0; i < lightCount; i++) {
            float ndotl = max(0.0, dot(n, lightDirs[vp * MAX_LIGHTS + i]));
            lit += lightColors[i] * lightIntensities[i] * ndotl;
        }
        col = albedoSample.rgb * lit;
    }

    // speed lines: fixed thin spokes with dashes racing OUTWARD from the
    // screen center (radial motion = forward speed, no rotation). Fade in
    // near top speed, full strength while boosting.
    float lineStrength = max(boostAmount[vp],
                             smoothstep(0.8, 1.0, speedAmount[vp]) * 0.4);
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
    if (boostAmount[vp] > 0.001) {
        float r = length(ndc);
        col *= 1.0 - 0.12 * boostAmount[vp] * smoothstep(0.8, 1.5, r);
    }

    finalColor = vec4(col, 1.0);
}
