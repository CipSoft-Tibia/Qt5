vec4 color;
void MAIN()
{
    color = gridColor;
}

void POST_PROCESS()
{
    COLOR_SUM = color;
}


