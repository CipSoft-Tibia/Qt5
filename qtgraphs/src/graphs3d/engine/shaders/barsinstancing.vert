VARYING vec3 pos;
VARYING vec3 instanceColor;

void MAIN()
{
    pos = VERTEX;
    instanceColor = INSTANCE_COLOR.rgb;
    POSITION = INSTANCE_MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
