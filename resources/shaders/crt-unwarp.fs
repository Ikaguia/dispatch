#version 330 core
in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // curved image
uniform vec2 curveStrength; // same magnitude used for curvature
uniform int iterations; // e.g. 6..10

// direct mapping used in forward shader (for reference):
// u' = u * (1 + (u.y^2) * CURVE)
// v' = v * (1 + (u.x^2) * CURVE)
// No easy closed-form inverse -> iterate

vec2 inverseCurve(vec2 uvp) {
    // uvp in [0,1] (destination / curved coords)
    // map to [-1,1]
    vec2 target = uvp * 2.0 - 1.0;
    // initial guess: the same (reasonable starting point)
    vec2 guess = target;
    // iterate: solve for guess such that curveRemap(guess) = target
    for (int i = 0; i < iterations; ++i) {
        // compute forward mapping of guess
        float gx = guess.x * (1.0 + (guess.y * guess.y) * curveStrength.x);
        float gy = guess.y * (1.0 + (guess.x * guess.x) * curveStrength.y);
        // compute delta
        vec2 delta = vec2(gx, gy) - target;
        // heuristic correction: divide by approx derivative magnitude
        // approximate Jacobian diagonal terms to scale the correction
        float denomY = 1.0 + (guess.x*guess.x) * curveStrength.x;
        float denomX = 1.0 + (guess.y*guess.y) * curveStrength.y;
        // update guess, small relaxation for stability
        guess.x -= delta.x / max(0.1, denomX);
        guess.y -= delta.y / max(0.1, denomY);
    }
    return guess * 0.5 + 0.5; // map back to [0,1]
}

void main() {
    // fragTexCoord is the destination pixel coordinate in [0,1]
    vec2 srcUV = inverseCurve(fragTexCoord);
    if (srcUV.x < 0.0 || srcUV.x > 1.0 || srcUV.y < 0.0 || srcUV.y > 1.0) {
        finalColor = vec4(0.0);
    } else {
        finalColor = texture(texture0, srcUV);
    }
}
