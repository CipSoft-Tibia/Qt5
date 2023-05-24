vec4 diffuse = vec4(0.0);
float shininess = 50.0;
float specularBrightness = 0.25;
// 0...1.0
float ambientBrightness = 0.75;
// 0...1.0
float directionalBrightness = 0.75;
// 0...1.0
float pointSize = 0.75f;
VARYING vec3 pos;

void MAIN()
{
    float distanceToCenter = distance(vec2(0.0), pos.xy);
    if (distanceToCenter > pointSize)
        discard;

    vec2 gradientUV = vec2(gradientPos, 0.0);
    vec3 color = texture(custex, gradientUV).xyz;
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
