vec4 desaturate(vec3 color, float strength)
{
    vec3 lum = vec3(0.299, 0.587, 0.114); // lum values based on: ITU-R BT.601
    vec3 gray = vec3(dot(lum, color));
    return vec4(mix(color, gray, -strength), 1.0);
}

void MAIN()
{
    vec4 sourceColor = texture(INPUT, INPUT_UV);
    vec4 basecolor = vec4(sourceColor.r * redStrength, sourceColor.g * greenStrength, sourceColor.b * blueStrength, sourceColor.a);
    FRAGCOLOR = desaturate(basecolor.rgb, saturation);
}
