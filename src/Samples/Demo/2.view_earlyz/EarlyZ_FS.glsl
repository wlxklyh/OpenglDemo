out vec4 FragColor;
in vec3 outColor;
in vec2 outTexCoord;

uniform sampler2D Texture0;

#if OPEN_EARLY_Z
layout(early_fragment_tests) in;
#endif

layout(binding = 0, offset = 0) uniform atomic_uint counter;
void main()
{
    // 计算纹理坐标的导数
    // vec2 texDx = dFdx(outTexCoord);
    // vec2 texDy = dFdy(outTexCoord);
    // 计算LOD
    // float lod = length(vec2(length(texDx), length(texDy)));
    //vec4 color0 = texture(Texture0,outTexCoord);
    // vec4 color0 = textureLod(Texture0, outTexCoord, lod);

    atomicCounterIncrement(counter);
    FragColor = vec4(outColor, 1.0);
}