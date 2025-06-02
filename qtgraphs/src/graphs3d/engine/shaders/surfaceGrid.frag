vec4 color;
void MAIN()
{
    if (abs(VAR_WORLD_POSITION.y) > graphHeight)
        discard;
    color = gridColor;
}

void POST_PROCESS()
{
    COLOR_SUM = color;
}


