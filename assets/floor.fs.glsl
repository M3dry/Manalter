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

    float a = clamp(smoothstep(radius, 0.0f, 1000.0f), 20.0f/255.0f, 64.0f/255.0f);
    return vec4(a, a, a, 1.0f);
}

void main() {
    // vec4 texelColor = texture(tex, fragTexCoord);
    // finalColor = texelColor*colDiffuse*fragColor;

    vec3 totalPosition = totalOrigin + fragTexCoord.x*vec3(totalMappingDims.x, 0.0f, 0.0f) + fragTexCoord.y*vec3(0.0f, 0.0f, totalMappingDims.y);
    finalColor = color(totalPosition);
};
