// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define PI 3.1415926
float grid;
vec3 gridDiff;

float CalculateGrid(vec3 dist, vec3 normal, vec3 pos)
{
    //only show grid on intended normal plane
    dist = clamp(dist + abs(normal), 0,1);
    //dont show grid on category axes
    dist = clamp(dist + abs(normal.zyx) *
                 vec3(float(xCategory), 0, float(zCategory)),
                 0,1);
    //dont show grid on bottom if gridOnTop
    dist = clamp(dist + abs(normal.y) * step(VAR_WORLD_POSITION.y, 0.5) * float(gridOnTop), 0, 1);

    vec3 diffX = dFdx(VAR_WORLD_POSITION);
    vec3 diffY = dFdy(VAR_WORLD_POSITION);
    vec3 posDeriv = vec3(
                length(vec2(diffX.x, diffY.x)),
                length(vec2(diffX.y, diffY.y)),
                length(vec2(diffX.z, diffY.z)));

    vec3 lineWidth = vec3(gridWidth);
    if (polar) {
        //keep angle line width more constant
        float angleWidth = lineWidth.x  * min(scale.x, scale.z)/ PI;
        lineWidth.x = angleWidth / max(pos.z, angleWidth);
        lineWidth.z = lineWidth.z * min(scale.x, scale.z) ;
        dist.x += step(1, pos.z);
        dist.z += step(1.05,pos.z);
    }
    vec3 drawWidth = max(lineWidth, posDeriv);
    vec3 lineAA = lineWidth * 0.5 + vec3(length(posDeriv)) * 1.5;
    vec3 grid3 = smoothstep(min(drawWidth + lineAA, 1), drawWidth - lineAA, dist);
    grid3 *= clamp(lineWidth.x / drawWidth.x, 0,1);

    float linesXZ = mix(grid3.x, 1.0, grid3.z);
    float lines = mix(linesXZ, 1.0, grid3.y) * float(gridVisible);

    return lines;
}

void MAIN()
{
    vec3 pos = VAR_WORLD_POSITION / (2 * scale) + 0.5;
    if (polar) {
        pos.xz = VAR_WORLD_POSITION.xz / min(scale.x, scale.z);
        float radius = sqrt((pos.x * pos.x) + (pos.z * pos.z));
        float theta = fract((atan(pos.z, pos.x) / PI) / 2.0 + 0.5) + 0.75;
        pos = vec3(theta , pos.y, radius);
    }

    vec3 dist = vec3(
                1 - texture(gridTex, vec2(pos.x, 0.1)).x,
                1 - texture(gridTex, vec2(pos.y, 0.1)).y,
                1 - texture(gridTex, vec2(pos.z, 0.1)).z
    );
    float gridLines = CalculateGrid(dist, NORMAL, pos);


    vec3 subDist = vec3(
                1 - texture(gridTex, vec2(pos.x, 0.9)).x,
                1 - texture(gridTex, vec2(pos.y, 0.9)).y,
                1 - texture(gridTex, vec2(pos.z, 0.9)).z
    );

    float subgridLines = CalculateGrid(subDist, NORMAL, pos);

    // combine grid and subgrid
    float totalGrid = min(gridLines + subgridLines, 1);
    float overlap = ceil(gridLines + subgridLines - 1);

    vec3 subColor = mix(baseColor.rgb, subgridLineColor.rgb, min(subgridLines + overlap, 1));
    vec3 gridColor = mix(subColor, gridLineColor.rgb, gridLines);

    //keep grid factor const if background not visible
    grid = mix(step(0.1, totalGrid), totalGrid, float(baseVisible));

    float alpha = mix(float(baseVisible),float(gridVisible), totalGrid);
    vec3 color = mix(baseColor.rgb, gridColor.rgb, grid);

    //remove backround from top grid
    if ((1 - totalGrid) * step(0.5, VAR_WORLD_POSITION.y * abs(NORMAL.y) * float(gridOnTop)) > 0.3)
        discard;

    if (alpha < 0.8)
        discard;

    gridDiff = color.rgb;
    BASE_COLOR = vec4(color.rgb, alpha);
    EMISSIVE_COLOR = color.rgb * 0.3;
    ROUGHNESS = 0.3;
}

void POST_PROCESS()
{
    vec3 sumBack = vec3(DIFFUSE.rgb + SPECULAR + EMISSIVE);
    vec3 sumGrid = gridDiff;
    COLOR_SUM = vec4(mix(sumBack, sumGrid, grid), DIFFUSE.a);
}
