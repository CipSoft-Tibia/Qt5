vec4 diffuse = vec4(0.0);
float ambientBrightness = 0.75;
float directionalBrightness = 0.75;
float pointSize = 0.75f;
VARYING vec3 pos;
VARYING vec3 vColor;

void MAIN()
{
    if (usePoint) {
        float distanceToCenter = distance(vec2(0.0), pos.xy);
        if (distanceToCenter > pointSize)
            discard;
    }

    vec3 color;
    vec2 gradientUV;
    switch (colorStyle) {
    case 0: // Uniform
        color = uColor.rgb;
        break;
    case 1: // Object gradient
        gradientUV = vec2((pos.y + 1.0) / 2.0, 0.0);
        color = texture(custex, gradientUV).xyz;
        break;
    case 2: //  Range gradient
        vec2 gradientUV = vec2((VAR_WORLD_POSITION.y + 1.0) / 2.0, 0.0);
        color = texture(custex, gradientUV).xyz;
        if (!usePoint)
            color = vColor;
        break;
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
    DIFFUSE += diffuse.rgb * directionalBrightness * LIGHT_COLOR * SHADOW_CONTRIB
            * vec3(max(0.0, dot(normalize(NORMAL), TO_LIGHT_DIR)));
}
