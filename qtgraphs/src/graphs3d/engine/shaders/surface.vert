VARYING vec3 pos;
VARYING vec2 UV;
out layout(location = 9) flat vec3 nF;

void MAIN()
{
    UV = UV0 * (vertices / range);

    vec2 uvR = UV + vec2(xDiff, 0.0);
    if (UV.x > (1.0 - xDiff))
        uvR = UV - vec2(xDiff, 0.0);
    vec2 uvU = UV + vec2(0.0, yDiff);
    if (UV.y > (2.0 - yDiff))
        uvU = UV - vec2(0.0, yDiff);
    vec3 v1 = texture(height, UV).rgb;
    vec3 v2 = texture(height, uvR).rgb;
    vec3 v3 = texture(height, uvU).rgb;
    vec3 v21 = v2 - v1;
    vec3 v31 = v3 - v1;
    vec3 n = cross(v21, v31);
    if (UV.x > (1.0 - xDiff))
        n = cross(v31, v21);
    if (UV.y > (1.0 - yDiff))
        n = cross(v31, v21);

    NORMAL = n;
    nF = n;
    VERTEX = v1;

    pos = VERTEX;
    vec4 pos = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
    pos.z += 0.000001;
    POSITION = pos;
}
