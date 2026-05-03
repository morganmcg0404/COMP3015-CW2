#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 NormalOut;
layout (location = 2) out vec4 ObjectIdOut;

uniform vec3 color;
uniform float objectId;

void main()
{
    FragColor = vec4(color, 1.0);
    // Emissive meshes do not provide normals in this pass; use a stable default.
    NormalOut = vec4(0.5, 0.5, 1.0, 1.0);
    ObjectIdOut = vec4(objectId, 0.0, 0.0, 1.0);
}