VARYING vec3 pos;
VARYING vec3 instanceColor;
VARYING float instanceAlpha;

void MAIN()
{
    pos = VERTEX;
    instanceColor = INSTANCE_COLOR.rgb;
    instanceAlpha = INSTANCE_COLOR.a;
    POSITION = INSTANCE_MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
