# Bloom Effect — Cleaned GLSL/C++ Code

Below is a cleaned version of the OCR output for the **Bloom Effect** section (pages 30–43). I corrected OCR mistakes (`retum` → `return`, malformed variable names, missing brackets, etc.) and reconstructed the code into a format that should compile with minor integration changes depending on your existing framework.

---

## Fragment Shader (`bloom.fs`)

```glsl
#version 460 core

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

uniform int Pass;

layout(binding = 0) uniform sampler2D HdrTex;
layout(binding = 1) uniform sampler2D BlurTex1;
layout(binding = 2) uniform sampler2D BlurTex2;

uniform float LumThresh;
uniform float PixOffset[10] = float[](0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0);
uniform float Weight[10];
uniform float Exposure = 0.35;
uniform float White = 0.928;
uniform float AveLum;

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

float luminance(vec3 color) {
    return 0.2126 * color.r +
           0.7152 * color.g +
           0.0722 * color.b;
}

vec4 pass1();
vec4 pass2();
vec4 pass3();
vec4 pass4();
vec4 pass5();

void main() {
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
```

---

## Pass 1 — Render Scene to HDR Texture

```glsl
vec4 pass1() {
    vec3 n = normalize(Normal);
    vec3 color = vec3(0.0);

    for(int i = 0; i < 3; i++) {
        color += blinnPhong(Position, n, i);
    }

    return vec4(color, 1.0);
}
```

---

## Pass 2 — Bright Pass Filter

```glsl
vec4 pass2() {
    vec4 val = texture(HdrTex, TexCoord);

    if(luminance(val.rgb) > LumThresh)
        return val;
    else
        return vec4(0.0);
}
```

---

## Pass 3 — Vertical Blur

```glsl
vec4 pass3() {
    float dy = 1.0 / textureSize(BlurTex1, 0).y;

    vec4 sum = texture(BlurTex1, TexCoord) * Weight[0];

    for(int i = 1; i < 10; i++) {
        sum += texture(BlurTex1,
              TexCoord + vec2(0.0, PixOffset[i] * dy)) * Weight[i];

        sum += texture(BlurTex1,
              TexCoord - vec2(0.0, PixOffset[i] * dy)) * Weight[i];
    }

    return sum;
}
```

---

## Pass 4 — Horizontal Blur

```glsl
vec4 pass4() {
    float dx = 1.0 / textureSize(BlurTex2, 0).x;

    vec4 sum = texture(BlurTex2, TexCoord) * Weight[0];

    for(int i = 1; i < 10; i++) {
        sum += texture(BlurTex2,
              TexCoord + vec2(PixOffset[i] * dx, 0.0)) * Weight[i];

        sum += texture(BlurTex2,
              TexCoord - vec2(PixOffset[i] * dx, 0.0)) * Weight[i];
    }

    return sum;
}
```

---

## Pass 5 — Composite + Tone Mapping

```glsl
vec4 pass5() {
    vec4 color = texture(HdrTex, TexCoord);

    vec3 xyzCol = rgb2xyz * vec3(color);

    float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;

    vec3 xyYCol = vec3(
        xyzCol.x / xyzSum,
        xyzCol.y / xyzSum,
        xyzCol.y
    );

    float L = (Exposure * xyYCol.z) / AveLum;
    L = (L * (1 + L / (White * White))) / (1 + L);

    xyzCol.x = (L * xyYCol.x) / xyYCol.y;
    xyzCol.y = L;
    xyzCol.z = (L * (1 - xyYCol.x - xyYCol.y)) / xyYCol.y;

    vec4 toneMapped = vec4(xyz2rgb * xyzCol, 1.0);

    vec4 blurTex = texture(BlurTex1, TexCoord);

    return toneMapped + blurTex;
}
```

---

# C++ Render Function (`SceneBasic_Uniform::render()`)

```cpp
void SceneBasic_Uniform::render()
{
    pass1();
    pass2();
    pass3();
    pass4();
    pass5();
}
```

---

# Pass Functions (C++)

```cpp
void SceneBasic_Uniform::pass1()
{
    prog.setUniform("Pass", 1);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawScene();
}

void SceneBasic_Uniform::pass2()
{
    prog.setUniform("Pass", 2);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        tex1,
        0
    );

    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SceneBasic_Uniform::pass3()
{
    prog.setUniform("Pass", 3);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        tex2,
        0
    );

    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SceneBasic_Uniform::pass4()
{
    prog.setUniform("Pass", 4);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        tex1,
        0
    );

    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SceneBasic_Uniform::pass5()
{
    prog.setUniform("Pass", 5);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
```

---

# Things You Still Need to Verify

Your framework may already define:
- `blinnPhong()`
- `drawScene()`
- `computeLogAveLuminance()`
- FBO creation (`hdrFBO`, `blurFBO`)
- `tex1`, `tex2`, `hdrTex`

Those weren’t fully visible in the OCR output, but the core bloom pipeline is now clean and should be much easier to integrate.

