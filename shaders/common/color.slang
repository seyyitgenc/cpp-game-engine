// Color encoding shared by the passes that write the scene image's values.

// The sRGB transfer function, for a target whose format does not apply it
// (the scene image is UNORM).
float3 LinearToSrgb(float3 color) {
    float3 low = color * 12.92;
    float3 high = 1.055 * pow(color, 1.0 / 2.4) - 0.055;
    return select(color <= 0.0031308, low, high);
}
