VARYING vec3 pos;
VARYING vec2 UV;

void MAIN()
{
    UV = UV0 * (vertCount / size);

    float xStep = xDiff;
    float yStep = yDiff;
    if (flipU)
        xStep *= -1;
    if (flipV)
        yStep *= -1;

    vec2 uStep = vec2(xStep, 0.0);
    vec2 vStep = vec2(0.0, yStep);

    vec3 v = texture(height, UV).xyz;

    vec3 vRight = texture(height, UV + uStep).xyz;
    vec3 vLeft = texture(height, UV - uStep).xyz;
    vec3 vUp = texture(height, UV + vStep).xyz;
    vec3 vDown = texture(height, UV - vStep).xyz;

    vec3 tangent = vLeft - vRight;
    vec3 bitangent = vUp - vDown;

    vec3 n = normalize(cross(bitangent, tangent));

    NORMAL = n;
    VERTEX = v;

    pos = VERTEX;
    vec4 pos = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
    pos.z += 0.000001;
    POSITION = pos;
}
