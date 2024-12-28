out vec4 FragColor;
in vec3 ourColor;

#if OPEN_EARLY_Z
layout(early_fragment_tests) in;
#endif

layout(binding = 0, offset = 0) uniform atomic_uint counter;
void main()
{
    atomicCounterIncrement(counter);
    FragColor = vec4(ourColor, 1.0);
}