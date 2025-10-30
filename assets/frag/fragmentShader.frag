#version 450
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main(){
    float t = fragTexCoord.y;
    vec3 top = vec3(0.0, 1.0, 0.0);
    vec3 bottom = vec3(0.0, 0.0, 1.0);
    vec3 color = mix(bottom, top, t);
    outColor = vec4(color, 1.0);
    //outColor = texture(texSampler, fragTexCoord);
}
