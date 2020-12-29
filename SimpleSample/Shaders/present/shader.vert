#version 450

layout(location = 0) out vec2 outUV;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    float x = float(gl_VertexIndex / 2);
    float y = float(gl_VertexIndex % 2);

    outUV = vec2(x, y);
    gl_Position = vec4(x * 2.f - 1.f, y * 2.f - 1.f, 0, 1.0);
}