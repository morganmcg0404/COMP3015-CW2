#version 460 core

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

uniform int Pass;

layout(binding = 0) uniform sampler2D HdrTex;
layout(binding = 1) uniform sampler2D BlurTex1;
layout(binding = 2) uniform sampler2D BlurTex2;

uniform float LumThresh = 1.5;
uniform float PixOffset[10] = float[](0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0);
uniform float Weight[10] = float[](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162, 
                                   0.0, 0.0, 0.0, 0.0, 0.0);
uniform float Exposure = 0.35;
uniform float White = 0.928;
uniform float AveLum = 0.5;

// HDR tone mapping matrices
uniform mat3 rgb2xyz = mat3(
    0.4124564, 0.2126729, 0.0193339,
    0.3575761, 0.7151522, 0.1191920,
    0.1804375, 0.0721750, 0.9503041
);

uniform mat3 xyz2rgb = mat3(
    3.2404542, -0.9692660, 0.0556434,
   -1.5371385,  1.8760108, -0.2040259,
   -0.4985314,  0.0415560, 1.0572252
);

float luminance(vec3 color)
{
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

// Phong lighting model
vec3 blinnPhong(vec3 position, vec3 norm)
{
    vec3 texColor = vec3(0.8, 0.8, 0.8);
    
    // Light direction (fixed sun-like light)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 viewDir = normalize(-position);
    vec3 halfDir = normalize(lightDir + viewDir);
    
    // Ambient
    vec3 ambient = 0.2 * texColor;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * texColor;
    
    // Specular
    float spec = pow(max(dot(norm, halfDir), 0.0), 64.0);
    vec3 specular = 0.5 * vec3(1.0) * spec;
    
    return ambient + diffuse + specular;
}

vec4 pass1()
{
    // Render scene with Blinn-Phong lighting
    vec3 n = normalize(Normal);
    vec3 color = blinnPhong(Position, n);
    
    return vec4(color, 1.0);
}

vec4 pass2()
{
    // Bright pass filter - extract bright pixels for bloom
    vec4 val = texture(HdrTex, TexCoord);
    
    if(luminance(val.rgb) > LumThresh)
        return val;
    else
        return vec4(0.0);
}

vec4 pass3()
{
    // Vertical Gaussian blur
    float dy = 1.0 / textureSize(BlurTex1, 0).y;
    
    vec4 sum = texture(BlurTex1, TexCoord) * Weight[0];
    
    for(int i = 1; i < 10; i++)
    {
        sum += texture(BlurTex1, TexCoord + vec2(0.0, PixOffset[i] * dy)) * Weight[i];
        sum += texture(BlurTex1, TexCoord - vec2(0.0, PixOffset[i] * dy)) * Weight[i];
    }
    
    return sum;
}

vec4 pass4()
{
    // Horizontal Gaussian blur
    float dx = 1.0 / textureSize(BlurTex2, 0).x;
    
    vec4 sum = texture(BlurTex2, TexCoord) * Weight[0];
    
    for(int i = 1; i < 10; i++)
    {
        sum += texture(BlurTex2, TexCoord + vec2(PixOffset[i] * dx, 0.0)) * Weight[i];
        sum += texture(BlurTex2, TexCoord - vec2(PixOffset[i] * dx, 0.0)) * Weight[i];
    }
    
    return sum;
}

vec4 pass5()
{
    // Composite: HDR with bloom + tone mapping
    vec4 color = texture(HdrTex, TexCoord);
    
    // Convert RGB to XYZ for tone mapping
    vec3 xyzCol = rgb2xyz * vec3(color);
    
    float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;
    
    vec3 xyYCol = vec3(
        xyzCol.x / xyzSum,
        xyzCol.y / xyzSum,
        xyzCol.y
    );
    
    // Reinhard tone mapping
    float L = (Exposure * xyYCol.z) / AveLum;
    L = (L * (1.0 + L / (White * White))) / (1.0 + L);
    
    xyzCol.x = (L * xyYCol.x) / xyYCol.y;
    xyzCol.y = L;
    xyzCol.z = (L * (1.0 - xyYCol.x - xyYCol.y)) / xyYCol.y;
    
    vec4 toneMapped = vec4(xyz2rgb * xyzCol, 1.0);
    
    // Add blurred bloom
    vec4 blurTex = texture(BlurTex1, TexCoord);
    
    return toneMapped + blurTex;
}

void main()
{
    if(Pass == 1)
        FragColor = pass1();
    else if(Pass == 2)
        FragColor = pass2();
    else if(Pass == 3)
        FragColor = pass3();
    else if(Pass == 4)
        FragColor = pass4();
    else if(Pass == 5)
        FragColor = pass5();
}
