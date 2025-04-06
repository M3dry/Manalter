#version 330

in vec4 fragColor;

uniform sampler2D circle;

out vec4 finalColor;

void main() {
    vec4 texColor = texture(circle, gl_PointCoord);

    finalColor = vec4(fragColor.rgb * texColor.rgb * 1.7, fragColor.a * texColor.a);
}
