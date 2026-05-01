#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float bloomStrength;
uniform bool vignetteEnabled;
uniform float manaRatio;
uniform float healthRatio;
uniform int scoreValue;
uniform int timeValue;
uniform bool paused;
uniform vec2 mousePos;

float rectMask(vec2 uv, vec2 minP, vec2 maxP)
{
    return step(minP.x, uv.x) * step(minP.y, uv.y) * step(uv.x, maxP.x) * step(uv.y, maxP.y);
}

float digitMask(int digit, vec2 uv)
{
    float a = rectMask(uv, vec2(0.18, 0.78), vec2(0.82, 0.88));
    float b = rectMask(uv, vec2(0.80, 0.52), vec2(0.90, 0.80));
    float c = rectMask(uv, vec2(0.80, 0.16), vec2(0.90, 0.48));
    float d = rectMask(uv, vec2(0.18, 0.06), vec2(0.82, 0.16));
    float e = rectMask(uv, vec2(0.10, 0.16), vec2(0.20, 0.48));
    float f = rectMask(uv, vec2(0.10, 0.52), vec2(0.20, 0.80));
    float g = rectMask(uv, vec2(0.18, 0.42), vec2(0.82, 0.52));

    if (digit == 0) return clamp(a + b + c + d + e + f, 0.0, 1.0);
    if (digit == 1) return clamp(b + c, 0.0, 1.0);
    if (digit == 2) return clamp(a + b + g + e + d, 0.0, 1.0);
    if (digit == 3) return clamp(a + b + g + c + d, 0.0, 1.0);
    if (digit == 4) return clamp(f + g + b + c, 0.0, 1.0);
    if (digit == 5) return clamp(a + f + g + c + d, 0.0, 1.0);
    if (digit == 6) return clamp(a + f + g + e + c + d, 0.0, 1.0);
    if (digit == 7) return clamp(a + b + c, 0.0, 1.0);
    if (digit == 8) return clamp(a + b + c + d + e + f + g, 0.0, 1.0);
    if (digit == 9) return clamp(a + b + c + d + f + g, 0.0, 1.0);
    return 0.0;
}

vec2 narrowDigitUV(vec2 uv)
{
    uv.x = (uv.x - 0.5) * 1.18 + 0.5;
    uv.y = (uv.y - 0.5) * 0.62 + 0.5;
    return uv;
}

float letterMask(int letter, vec2 uv)
{
    float v = 0.0;

    if (letter == 0) // R
    {
        v = max(v, rectMask(uv, vec2(0.12, 0.10), vec2(0.24, 0.90)));
        v = max(v, rectMask(uv, vec2(0.24, 0.78), vec2(0.72, 0.90)));
        v = max(v, rectMask(uv, vec2(0.24, 0.46), vec2(0.68, 0.58)));
        v = max(v, rectMask(uv, vec2(0.62, 0.46), vec2(0.74, 0.78)));
        v = max(v, rectMask(uv, vec2(0.38, 0.10), vec2(0.74, 0.52)));
    }
    else if (letter == 1) // E
    {
        v = max(v, rectMask(uv, vec2(0.12, 0.10), vec2(0.24, 0.90)));
        v = max(v, rectMask(uv, vec2(0.24, 0.78), vec2(0.78, 0.90)));
        v = max(v, rectMask(uv, vec2(0.24, 0.46), vec2(0.66, 0.58)));
        v = max(v, rectMask(uv, vec2(0.24, 0.10), vec2(0.78, 0.22)));
    }
    else if (letter == 2) // S
    {
        v = max(v, rectMask(uv, vec2(0.18, 0.78), vec2(0.82, 0.90)));
        v = max(v, rectMask(uv, vec2(0.12, 0.46), vec2(0.74, 0.58)));
        v = max(v, rectMask(uv, vec2(0.18, 0.10), vec2(0.82, 0.22)));
        v = max(v, rectMask(uv, vec2(0.12, 0.46), vec2(0.24, 0.90)));
        v = max(v, rectMask(uv, vec2(0.76, 0.10), vec2(0.88, 0.54)));
    }
    else if (letter == 3) // U
    {
        v = max(v, rectMask(uv, vec2(0.12, 0.22), vec2(0.24, 0.90)));
        v = max(v, rectMask(uv, vec2(0.76, 0.22), vec2(0.88, 0.90)));
        v = max(v, rectMask(uv, vec2(0.24, 0.10), vec2(0.76, 0.22)));
    }
    else if (letter == 4) // M
    {
        v = max(v, rectMask(uv, vec2(0.12, 0.10), vec2(0.24, 0.90)));
        v = max(v, rectMask(uv, vec2(0.76, 0.10), vec2(0.88, 0.90)));
        v = max(v, rectMask(uv, vec2(0.24, 0.62), vec2(0.36, 0.90)));
        v = max(v, rectMask(uv, vec2(0.64, 0.62), vec2(0.76, 0.90)));
    }
    else if (letter == 5) // Q
    {
        v = max(v, rectMask(uv, vec2(0.18, 0.82), vec2(0.82, 0.90)));
        v = max(v, rectMask(uv, vec2(0.12, 0.22), vec2(0.24, 0.82)));
        v = max(v, rectMask(uv, vec2(0.76, 0.22), vec2(0.88, 0.82)));
        v = max(v, rectMask(uv, vec2(0.18, 0.10), vec2(0.82, 0.18)));
        v = max(v, rectMask(uv, vec2(0.60, 0.10), vec2(0.82, 0.32)));
    }
    else if (letter == 6) // I
    {
        v = max(v, rectMask(uv, vec2(0.18, 0.82), vec2(0.82, 0.90)));
        v = max(v, rectMask(uv, vec2(0.42, 0.10), vec2(0.58, 0.90)));
        v = max(v, rectMask(uv, vec2(0.18, 0.10), vec2(0.82, 0.18)));
    }
    else if (letter == 7) // T
    {
        v = max(v, rectMask(uv, vec2(0.18, 0.82), vec2(0.82, 0.90)));
        v = max(v, rectMask(uv, vec2(0.42, 0.10), vec2(0.58, 0.90)));
    }

    return clamp(v, 0.0, 1.0);
}

float drawLetter(int letter, vec2 uv, vec2 minP, vec2 maxP)
{
    vec2 p = (uv - minP) / (maxP - minP);
    if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
        return 0.0;
    return letterMask(letter, p);
}

void main()
{
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloom = texture(bloomBlur, TexCoords).rgb;

    float vignette = 1.0;
    if (vignetteEnabled)
    {
        vec2 centered = TexCoords - vec2(0.5);
        vignette = 1.0 - dot(centered, centered) * 1.35;
        vignette = clamp(vignette, 0.72, 1.0);
    }

    vec3 finalColor = (hdrColor + bloom * bloomStrength) * vignette;

    // Cross-shaped crosshair in the center of the screen, corrected for aspect ratio
    vec2 p = TexCoords - vec2(0.5);
    p.x *= 16.0 / 9.0;

    float hLine = step(abs(p.y), 0.0015) * step(abs(p.x), 0.012);
    float vLine = step(abs(p.x), 0.0015) * step(abs(p.y), 0.012);
    float crosshair = clamp(hLine + vLine, 0.0, 1.0);

    finalColor = mix(finalColor, vec3(1.0), crosshair);

    // Mana bar at bottom of the screen, filling left to right
    float mana = clamp(manaRatio, 0.0, 1.0);
    vec2 barMin = vec2(0.30, 0.03);
    vec2 barMax = vec2(0.70, 0.06);
    bool inBar = TexCoords.x >= barMin.x && TexCoords.x <= barMax.x && TexCoords.y >= barMin.y && TexCoords.y <= barMax.y;

    if (inBar)
    {
        vec3 bgColor = vec3(0.12, 0.12, 0.12);
        vec3 fillColor = vec3(0.20, 0.55, 1.00);
        float fillEdge = barMin.x + (barMax.x - barMin.x) * mana;

        if (TexCoords.x <= fillEdge)
            finalColor = fillColor;
        else
            finalColor = bgColor;
    }

    // Health bar above mana bar
    float health = clamp(healthRatio, 0.0, 1.0);
    vec2 healthBarMin = vec2(0.30, 0.07);
    vec2 healthBarMax = vec2(0.70, 0.10);
    bool inHealthBar = TexCoords.x >= healthBarMin.x && TexCoords.x <= healthBarMax.x && TexCoords.y >= healthBarMin.y && TexCoords.y <= healthBarMax.y;

    if (inHealthBar)
    {
        vec3 bgColor = vec3(0.12, 0.12, 0.12);
        vec3 fillColor = vec3(0.90, 0.20, 0.20);  // Red color for health
        float fillEdge = healthBarMin.x + (healthBarMax.x - healthBarMin.x) * health;

        if (TexCoords.x <= fillEdge)
            finalColor = fillColor;
        else
            finalColor = bgColor;
    }

    // Score tracker at the top of the screen
    vec2 scorePanelMin = vec2(0.36, 0.935);
    vec2 scorePanelMax = vec2(0.60, 0.99);
    bool inScorePanel = TexCoords.x >= scorePanelMin.x && TexCoords.x <= scorePanelMax.x && TexCoords.y >= scorePanelMin.y && TexCoords.y <= scorePanelMax.y;

    if (inScorePanel)
    {
        finalColor = vec3(0.08, 0.08, 0.08);

        int score = clamp(scoreValue, 0, 999999);
        int digit0 = score % 10;
        int digit1 = (score / 10) % 10;
        int digit2 = (score / 100) % 10;
        int digit3 = (score / 1000) % 10;
        int digit4 = (score / 10000) % 10;
        int digit5 = (score / 100000) % 10;

        int visibleDigits = 1;
        if (score >= 100000) visibleDigits = 6;
        else if (score >= 10000) visibleDigits = 5;
        else if (score >= 1000) visibleDigits = 4;
        else if (score >= 100) visibleDigits = 3;
        else if (score >= 10) visibleDigits = 2;

        vec2 panelUV = (TexCoords - scorePanelMin) / (scorePanelMax - scorePanelMin);
        vec2 innerUV = (panelUV - vec2(0.01, 0.12)) / vec2(0.98, 0.76);
        vec3 scoreColor = vec3(1.0, 0.92, 0.25);

        float digitWidth = 0.145;
        float digitGap = 0.00375;
        float digitHeight = 0.78;
        float digitY = 0.11;

        int digits[6] = int[6](digit5, digit4, digit3, digit2, digit1, digit0);
        float totalDigitWidth = float(visibleDigits) * digitWidth + float(max(visibleDigits - 1, 0)) * digitGap;
        float contentStart = 1.0 - 0.01 - totalDigitWidth;
        int startIndex = 6 - visibleDigits;

        for (int i = startIndex; i < 6; ++i)
        {
            int slot = i - startIndex;
            vec2 digitUV = innerUV;
            digitUV.x = (digitUV.x - (contentStart + (digitWidth + digitGap) * float(slot))) / digitWidth;
            digitUV.y = (digitUV.y - digitY) / digitHeight;
            digitUV = narrowDigitUV(digitUV);
            if (digitUV.x >= 0.0 && digitUV.x <= 1.0 && digitUV.y >= 0.0 && digitUV.y <= 1.0)
            {
                if (digitMask(digits[i], digitUV) > 0.5)
                    finalColor = scoreColor;
            }
        }
    }

    // Time tracker at the top-right of the screen
    vec2 timePanelMin = vec2(0.63, 0.935);
    vec2 timePanelMax = vec2(0.98, 0.99);
    bool inTimePanel = TexCoords.x >= timePanelMin.x && TexCoords.x <= timePanelMax.x && TexCoords.y >= timePanelMin.y && TexCoords.y <= timePanelMax.y;

    if (inTimePanel)
    {
        finalColor = vec3(0.08, 0.08, 0.08);

        int totalSeconds = clamp(timeValue, 0, 359999);
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds / 60) % 60;
        int seconds = totalSeconds % 60;

        int hour0 = hours % 10;
        int hour1 = (hours / 10) % 10;
        int minute0 = minutes % 10;
        int minute1 = (minutes / 10) % 10;
        int second0 = seconds % 10;
        int second1 = (seconds / 10) % 10;

        vec2 panelUV = (TexCoords - timePanelMin) / (timePanelMax - timePanelMin);
        vec2 innerUV = (panelUV - vec2(0.01, 0.12)) / vec2(0.98, 0.76);
        vec3 timeColor = vec3(0.70, 0.90, 1.0);

        float digitWidth = 0.10;
        float digitGap = 0.008;
        float digitHeight = 0.78;
        float digitY = 0.11;

        int digits[6] = int[6](hour1, hour0, minute1, minute0, second1, second0);
        float totalDigitWidth = float(8) * digitWidth + float(7) * digitGap;
        float contentStart = (1.0 - totalDigitWidth) * 0.5;

        for (int slot = 0; slot < 8; ++slot)
        {
            vec2 digitUV = innerUV;
            digitUV.x = (digitUV.x - (contentStart + (digitWidth + digitGap) * float(slot))) / digitWidth;
            digitUV.y = (digitUV.y - digitY) / digitHeight;
            digitUV = narrowDigitUV(digitUV);
            if (digitUV.x >= 0.0 && digitUV.x <= 1.0 && digitUV.y >= 0.0 && digitUV.y <= 1.0)
            {
                if (slot == 2 || slot == 5)
                {
                    float topDot = rectMask(digitUV, vec2(0.35, 0.68), vec2(0.65, 0.82));
                    float bottomDot = rectMask(digitUV, vec2(0.35, 0.18), vec2(0.65, 0.32));
                    if (topDot > 0.0 || bottomDot > 0.0)
                        finalColor = timeColor;
                }
                else
                {
                    int digitIndex = slot;
                    if (slot > 2)
                        digitIndex -= 1;
                    if (slot > 5)
                        digitIndex -= 1;

                    if (digitMask(digits[digitIndex], digitUV) > 0.5)
                    finalColor = timeColor;
                }
            }
        }
    }

    if (paused)
    {
        finalColor = mix(finalColor, vec3(0.0), 0.60);

        vec2 pausePanelMin = vec2(0.34, 0.28);
        vec2 pausePanelMax = vec2(0.66, 0.72);
        bool inPausePanel = TexCoords.x >= pausePanelMin.x && TexCoords.x <= pausePanelMax.x && TexCoords.y >= pausePanelMin.y && TexCoords.y <= pausePanelMax.y;
        if (inPausePanel)
            finalColor = mix(finalColor, vec3(0.05, 0.05, 0.05), 0.85);

        vec2 resumeMin = vec2(0.41, 0.52);
        vec2 resumeMax = vec2(0.59, 0.60);
        vec2 quitMin = vec2(0.41, 0.40);
        vec2 quitMax = vec2(0.59, 0.48);

        bool overResume = mousePos.x >= resumeMin.x && mousePos.x <= resumeMax.x && mousePos.y >= resumeMin.y && mousePos.y <= resumeMax.y;
        bool overQuit = mousePos.x >= quitMin.x && mousePos.x <= quitMax.x && mousePos.y >= quitMin.y && mousePos.y <= quitMax.y;

        vec3 resumeColor = overResume ? vec3(0.35, 0.80, 0.35) : vec3(0.22, 0.60, 0.22);
        vec3 quitColor = overQuit ? vec3(0.90, 0.30, 0.30) : vec3(0.70, 0.18, 0.18);

        if (TexCoords.x >= resumeMin.x && TexCoords.x <= resumeMax.x && TexCoords.y >= resumeMin.y && TexCoords.y <= resumeMax.y)
        {
            finalColor = resumeColor;
            float textMask = 0.0;
            vec2 base = vec2(0.418, 0.536);
            float letterW = 0.020;
            float letterH = 0.048;
            float gap = 0.0035;
            textMask = max(textMask, drawLetter(0, TexCoords, base + vec2(0.000, 0.0), base + vec2(letterW, letterH)));
            textMask = max(textMask, drawLetter(1, TexCoords, base + vec2((letterW + gap) * 1.0, 0.0), base + vec2((letterW + gap) * 1.0 + letterW, letterH)));
            textMask = max(textMask, drawLetter(2, TexCoords, base + vec2((letterW + gap) * 2.0, 0.0), base + vec2((letterW + gap) * 2.0 + letterW, letterH)));
            textMask = max(textMask, drawLetter(3, TexCoords, base + vec2((letterW + gap) * 3.0, 0.0), base + vec2((letterW + gap) * 3.0 + letterW, letterH)));
            textMask = max(textMask, drawLetter(4, TexCoords, base + vec2((letterW + gap) * 4.0, 0.0), base + vec2((letterW + gap) * 4.0 + letterW, letterH)));
            textMask = max(textMask, drawLetter(1, TexCoords, base + vec2((letterW + gap) * 5.0, 0.0), base + vec2((letterW + gap) * 5.0 + letterW, letterH)));
            if (textMask > 0.0)
                finalColor = vec3(0.95, 0.95, 0.95);
        }

        if (TexCoords.x >= quitMin.x && TexCoords.x <= quitMax.x && TexCoords.y >= quitMin.y && TexCoords.y <= quitMax.y)
        {
            finalColor = quitColor;
            float textMask = 0.0;
            vec2 base = vec2(0.439, 0.416);
            float letterW = 0.020;
            float letterH = 0.048;
            float gap = 0.0035;
            textMask = max(textMask, drawLetter(5, TexCoords, base + vec2(0.000, 0.0), base + vec2(letterW, letterH)));
            textMask = max(textMask, drawLetter(6, TexCoords, base + vec2((letterW + gap) * 1.0, 0.0), base + vec2((letterW + gap) * 1.0 + letterW, letterH)));
            textMask = max(textMask, drawLetter(7, TexCoords, base + vec2((letterW + gap) * 2.0, 0.0), base + vec2((letterW + gap) * 2.0 + letterW, letterH)));
            textMask = max(textMask, drawLetter(7, TexCoords, base + vec2((letterW + gap) * 3.0, 0.0), base + vec2((letterW + gap) * 3.0 + letterW, letterH)));
            if (textMask > 0.0)
                finalColor = vec3(0.95, 0.95, 0.95);
        }
    }

    // Keep bloom subtle so the scene does not wash out
    FragColor = vec4(finalColor, 1.0);
}