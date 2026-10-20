// Cook-Torrance BRDF for the glTF metallic-roughness model, and the punctual
// light evaluation that feeds it. Everything here works in world space and in
// linear light; exposure, tonemapping and gamma come later, once, in
// post-processing.

import common.frame;

static const float PI = 3.14159265359;

// Below this, a point light's highlight shrinks to a sub-pixel spike that
// aliases, and D() grows large enough to overflow a 16 bit float target.
// Filament clamps at the same value.
static const float MIN_ROUGHNESS = 0.045;

// What the lighting needs to know about one point on a surface.
struct Surface {
    float3 position;  // world space
    float3 normal;    // N, unit length
    float3 view;      // V, unit length, from the surface towards the camera
    float3 baseColor; // linear
    float metallic;
    float roughness;  // perceptual, as authored; squared into alpha below
};

// ----------
// BRDF terms
// ----------

// GGX / Trowbridge-Reitz normal distribution: the share of microfacets whose
// normal is H, i.e. that reflect L straight into V.
float D_GGX(float NdotH, float alpha) {
    float a2 = alpha * alpha;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Schlick-GGX approximation of Smith's masking-shadowing for one direction.
float G_SchlickGGX(float NdotX, float k) {
    return NdotX / (NdotX * (1.0 - k) + k);
}

// Smith's separable form: light blocked on the way in (L) times on the way out
// (V). k = (roughness + 1)^2 / 8 is UE4's remapping for punctual lights; image
// based lighting will want k = roughness^2 / 2 instead.
float G_Smith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}

// Schlick's Fresnel: reflectance climbs from F0 head on to 1 at grazing angles.
float3 F_Schlick(float3 F0, float VdotH) {
    return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

// Reflected radiance per unit of incoming irradiance from direction L.
float3 BRDF(Surface s, float3 L) {
    float3 H = normalize(s.view + L);

    // NdotV can go slightly negative where interpolated normals face away from
    // the camera; clamping to a small positive value keeps the division finite.
    float NdotV = max(dot(s.normal, s.view), 1e-4);
    float NdotL = saturate(dot(s.normal, L));
    float NdotH = saturate(dot(s.normal, H));
    float VdotH = saturate(dot(s.view, H));

    float roughness = max(s.roughness, MIN_ROUGHNESS);
    float alpha = roughness * roughness;

    // Dielectrics reflect ~4% head on, whatever their color. Metals reflect
    // their base color and have no diffuse at all.
    float3 F0 = lerp(float3(0.04), s.baseColor, s.metallic);
    float3 F = F_Schlick(F0, VdotH);

    float D = D_GGX(NdotH, alpha);
    float G = G_Smith(NdotV, NdotL, roughness);
    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);

    // Energy conservation: whatever Fresnel reflects is not available to the
    // diffuse lobe, and metals absorb what they do not reflect.
    float3 kD = (1.0 - F) * (1.0 - s.metallic);
    float3 diffuse = kD * s.baseColor / PI;

    return diffuse + specular;
}

// ---------------
// Punctual lights
// ---------------

// Inverse square falloff. With a range, it is also windowed down to exactly
// zero at that distance (Karis 2013), so the light can be skipped beyond it
// without a visible edge. Distance is floored at 1cm: a light sitting on the
// surface would otherwise divide by zero.
float DistanceAttenuation(float distanceSquared, float range) {
    float attenuation = 1.0 / max(distanceSquared, 0.0001);
    if (range > 0.0) {
        float ratio = distanceSquared / (range * range);
        float window = saturate(1.0 - ratio * ratio);
        attenuation *= window * window;
    }
    return attenuation;
}

// KHR_lights_punctual's cone falloff: 1 inside the inner cone, 0 outside the
// outer one, smooth in between. scale and offset come precomputed from the
// cone angles (Renderer::MakeGpuLight).
float SpotAttenuation(float3 spotDirection, float3 L, float scale, float offset) {
    float cosAngle = dot(spotDirection, -L);
    float attenuation = saturate(cosAngle * scale + offset);
    return attenuation * attenuation;
}

// Outgoing radiance towards the camera due to one light.
float3 EvaluateLight(GpuLight light, Surface s) {
    float3 L;
    float attenuation = 1.0;

    if (light.type == LIGHT_TYPE_DIRECTIONAL) {
        L = -light.direction;
    } else {
        float3 toLight = light.position - s.position;
        float distanceSquared = dot(toLight, toLight);
        L = toLight * rsqrt(max(distanceSquared, 1e-8));
        attenuation = DistanceAttenuation(distanceSquared, light.range);

        if (light.type == LIGHT_TYPE_SPOT) {
            attenuation *= SpotAttenuation(light.direction, L, light.spotAngleScale, light.spotAngleOffset);
        }
    }

    float NdotL = saturate(dot(s.normal, L));
    if (NdotL <= 0.0 || attenuation <= 0.0) return float3(0.0);

    float3 radiance = light.color * light.intensity * attenuation;
    return BRDF(s, L) * radiance * NdotL;
}
