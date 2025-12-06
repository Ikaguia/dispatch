#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;

// The pixel resolution of the drawn area
uniform vec2 resolution;

// Circle center and radius in pixels
uniform vec2 center;
uniform float radius;

// Soft fade at the boundary (0 = sharp edge)
uniform float attenuation;

// Optional texture scroll/offset
uniform vec2 texOffset;

void main() {
	// Pixel coordinate inside the destination rectangle
	vec2 pixel = fragTexCoord * resolution;

	float dist = distance(pixel, center);
	float edge = radius;

	// Circle mask: 1.0 inside, fades out over attenuation
	float alpha = 1.0 - smoothstep(edge - attenuation, edge, dist);

	if (alpha <= 0.0) discard;

	vec2 uv = fragTexCoord + texOffset;

	vec4 texColor = texture(texture0, uv);
	texColor.a *= alpha;
	finalColor = texColor;
}
