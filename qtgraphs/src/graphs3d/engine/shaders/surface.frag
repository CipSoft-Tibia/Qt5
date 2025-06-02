vec4 diffuse = vec4(0.0);
float ambientBrightness = 0.75; // 0...1.0
float directionalBrightness = 0.50; // 0...1.0
VARYING vec3 pos;
VARYING vec2 UV;

void MAIN()
{
    if (any(greaterThan(UV, vec2(1.01))) || abs(VAR_WORLD_POSITION.y) > graphHeight)
        discard;
    vec3 color;
    vec2 gradientUV;
    switch (colorStyle) {
    case 0: //Object gradient
        gradientUV = vec2(gradientMin + pos.y * gradientHeight, 0.0);
        color = texture(custex, gradientUV).xyz;
        break;
    case 1: //Range gradient
        gradientUV = vec2((VAR_WORLD_POSITION.y + 1.0) / 2.0, 0.0);
        color = texture(custex, gradientUV).xyz;
        break;
    case 2: // Uniform color
        color = uniformColor.rgb;
        break;
    case 3: // Textured model
        vec2 offsetNormalized = uvOffset * (1 / (vertCount - 1));
        vec2 texUV = UV0 + offsetNormalized;
        if (flipU)
            texUV.x = 1 - texUV.x;
        if (flipV)
            texUV.y = 1 - texUV.y;
        color = texture(baseColor, texUV).xyz;
        break;
    }

    if (flatShading) {
        vec3 dpdx = dFdx(VAR_WORLD_POSITION);
        vec3 dpdy = dFdy(VAR_WORLD_POSITION);
        vec3 n = normalize(cross(dpdy,dpdx));
        if (NEAR_CLIP_VALUE < 0.0) //effectively: if openGL
            n = normalize(cross(dpdx,dpdy));
        NORMAL = n;
    }
    diffuse = vec4(color, 1.0);
    BASE_COLOR = diffuse;
}

void AMBIENT_LIGHT()
{
    DIFFUSE += diffuse.rgb * TOTAL_AMBIENT_COLOR * ambientBrightness;
}

void DIRECTIONAL_LIGHT()
{
    DIFFUSE += diffuse.rgb * directionalBrightness * LIGHT_COLOR * SHADOW_CONTRIB * vec3(max(0.0, dot(normalize(NORMAL), TO_LIGHT_DIR)));
}

void SPECULAR_LIGHT()
{
    vec3 H = normalize(VIEW_VECTOR + TO_LIGHT_DIR);
    float cosAlpha = max(0.0, dot(H, normalize(NORMAL)));
    float shine = pow(cosAlpha, shininess);
    const vec3 specularColor = vec3(specularBrightness);
    SPECULAR += shine * specularColor;
}
