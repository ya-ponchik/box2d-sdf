#version 330

out vec4 FragColor;

uniform float pixel_to_meter_ratio;
uniform vec2 center;
uniform float test_time;

// THIS COMMENT WILL BE REPLACED BY CODE AT RUNTIME

void main()
{
    float d = calc_sdf(gl_FragCoord.xy * pixel_to_meter_ratio - center);

    // draw blurry shape and tone down the color at exterior
    vec3 col = vec3(0.08, 0.47, 0.27);
    if (d > 0.0) col *= 0.6 * exp(-0.1 * d);
    // apply stripes
    col *= 0.5 + 0.3 * cos(5.0 * d);

    float outline_d = abs(d + pixel_to_meter_ratio) - pixel_to_meter_ratio;
    float tmp = fwidth(outline_d * 1.5) * 0.5;
    float alpha = smoothstep(tmp, -tmp, outline_d);

    // draw an outline on top
	col = mix(col, vec3(0.09, 0.86, 0.47), alpha);

	FragColor = vec4(col, 1.0);
}
