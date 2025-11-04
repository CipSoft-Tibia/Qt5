VARYING vec3 pos;
void MAIN()
{
    vec2 UV = UV0 * (vertices / range);
    VERTEX = texture(height, UV).rgb;
    pos = VERTEX;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
