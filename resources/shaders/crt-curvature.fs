#version 330 core
in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 curveStrength; // 0..1

vec2 curveRemap(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    uv.x *= 1.0 + (uv.y * uv.y) * curveStrength.x;
    uv.y *= 1.0 + (uv.x * uv.x) * curveStrength.y;
    uv = uv * 0.5 + 0.5;
    return uv;
}

void main() {
    vec2 uv = curveRemap(fragTexCoord);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) finalColor = vec4(0.0); // black outside
    else finalColor = texture(texture0, uv);
}
