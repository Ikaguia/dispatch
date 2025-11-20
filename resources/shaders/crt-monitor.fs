#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;

// ---- PARAMETERS ----
const vec2 CURVE = {0.014, 0.048};        // screen curvature strength
const float SCAN_INTENSITY = 0.08;
const float SCAN_SPEED_DIV = 400;
const float VIGNETTE_INTENSITY = 0.48;

vec2 curveRemap(vec2 uv) {
    uv = uv * 2.0 - 1.0;         // map to [-1,1]
    uv.x *= 1.0 + (uv.y * uv.y) * CURVE.x;
    uv.y *= 1.0 + (uv.x * uv.x) * CURVE.y;
    uv = uv * 0.5 + 0.5;
    return uv;
}

void main() {
    vec2 uv = curveRemap(fragTexCoord);

    // Out of bounds? Darken the edges like old CRTs.
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 color = texture(texture0, uv).rgb;

    // Scanlines
    float scan = sin((uv.y + time / SCAN_SPEED_DIV) * 1200.0) * SCAN_INTENSITY;
    color -= scan;

    // // Vignette
    // float vignette = pow(uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y), 
    //                      VIGNETTE_INTENSITY);
    // color *= vignette * 5.0;

    finalColor = vec4(color, 1.0);
}
