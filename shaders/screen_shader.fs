#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform sampler2D guiTexture;
const float offset = 1.0 / 300.0;

void main() {
    vec4 sceneColor = texture(sceneTexture, TexCoords);
    vec4 guiColor = texture(guiTexture, TexCoords);

    // Simple additive blending
    // FragColor = sceneColor + guiColor;

    // Alpha blending
    FragColor = mix(sceneColor, guiColor, guiColor.a);
    
    // 1. example
    
    //  FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);
    // 2. example
    // float average = (FragColor.r + FragColor.g + FragColor.b) / 3.0;
    // FragColor = vec4(average, average, average, 1.0);
}