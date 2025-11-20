#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float time;       // passed from C++
uniform vec2 resolution;  // resolution of the map area
uniform float speed;      // cloud scroll speed
uniform float density;    // controls how thick clouds are

//-----------------------------------------------------
// Noise functions
//-----------------------------------------------------

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
           (c - a) * u.y * (1.0 - u.x) +
           (d - b) * u.x * u.y;
}

float fbm(vec2 p) {
    float f = 0.0;
    float s = 0.5;

    for (int i = 0; i < 5; i++) {
        f += s * noise(p);
        p *= 2.0;
        s *= 0.5;
    }
    return f;
}

//-----------------------------------------------------

void main() {
    vec2 uv = fragTexCoord * resolution / 300.0;

    // scroll
    uv.x += time * speed;

    float n = fbm(uv);

    float cloud = smoothstep(density, 1.0, n);

    vec3 cloudColor = vec3(1.0); // white clouds

    finalColor = vec4(cloudColor, cloud * 0.6);
}
