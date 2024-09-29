#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

float random(vec2 uv) {
    return fract(sin(dot(uv, vec2(21.452, 91.842))) * 44701.4128);
}

void main() {
    const vec3 dark_green = vec3(35./255., 148./255., 65./255.);
    const vec3 light_green = vec3(28./255., 237./255., 84./255.);

    float noise = random(fragTexCoord * 10.);
    vec3 color = mix(dark_green, light_green, fragTexCoord.y + noise  * 0.2);

    finalColor = vec4(color, 1.);
}
