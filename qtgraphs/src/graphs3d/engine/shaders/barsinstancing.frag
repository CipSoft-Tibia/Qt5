vec4 diffuse = vec4(0.0);
float ambientBrightness = 0.75; // 0...1.0
float directionalBrightness = 0.75; // 0...1.0

VARYING vec3 pos;
VARYING vec3 instanceColor;

void MAIN()
{
    vec3 color;
    vec2 gradientUV;
    switch(colorStyle) {
    case 0: //uniform
        gradientUV = vec2((pos.y + 1.0) / 2.0, 0.0); //for highlight only
        color = normalize(uniformColor.rgb);
        color *= instanceColor;
        break;
    case 1: // objectgradient
        gradientUV = vec2((pos.y + 1.0) / 2.0, 0.0);
        color = texture(custex, gradientUV).xyz;
        break;
    case 2: //rangegradient
        gradientUV = vec2((VAR_WORLD_POSITION.y + 1.0) / 2.0, 0.0);
        color = texture(custex, gradientUV).xyz;
        break;
    }
    if (isHighlight)
        color = texture(custex, gradientUV).xyz;

    diffuse = vec4(color, 1.0);
    BASE_COLOR = diffuse;
}

void AMBIENT_LIGHT()
{
    if (colorStyle == 0)
        DIFFUSE += diffuse.rgb * ambientBrightness;
    else
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
