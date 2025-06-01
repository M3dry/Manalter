#version 330

#define PI 3.14

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D tex;
uniform vec4 colDiffuse;

uniform vec3 totalOrigin; // top left corner, i.e. fragTexCoord = (0, 1)
uniform vec2 totalMappingDims; // .x = width, .y = height, i.e. totalPosition.x + width == top right corner, ...

vec4 color(vec3 totalPosition) {
    float radius = sqrt(totalPosition.x*totalPosition.x + totalPosition.z*totalPosition.z);
    float alpha = atan(totalPosition.z, totalPosition.x);

    float r = step(100, radius)*255;
    if (r == 0 && alpha <= 0) {
        r = 255;
    }
    // return vec4(r, step(100, radius)*255, step(100, radius)*255, 255);
    return vec4(smoothstep(0, 1000, radius)/1000.0f * 255.0f, smoothstep(0, 1000, radius)/1000.0f * 255.0f, smoothstep(0, 1000, radius)/1000.0f * 255.0f, 255);
}

void main() {
    // vec4 texelColor = texture(tex, fragTexCoord);
    // finalColor = texelColor*colDiffuse*fragColor;

    vec3 totalPosition = totalOrigin + fragTexCoord.x*vec3(totalMappingDims.x, 0.0f, 0.0f) + fragTexCoord.y*vec3(0.0f, 0.0f, totalMappingDims.y);
    finalColor = color(totalPosition);
};
