vec4 color;
VARYING vec3 pos;
void MAIN()
{
    if (abs(pos.y) > graphHeight)
        discard;
    color = gridColor;
}

void POST_PROCESS()
{
    COLOR_SUM = color;
}


