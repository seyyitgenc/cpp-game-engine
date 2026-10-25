// The G-buffer contract between geometry_pass (writes it) and lighting_pass
// (reads it). GeometryPass.hpp creates the attachments with these formats, in
// this order:
//
//   0  R8G8B8A8_SRGB        rgb base color. Written and read as linear; the
//                           sRGB storage only spends the 8 bits where the eye
//                           needs them, in the darks.
//   1  R16G16_SFLOAT        world space normal, octahedral encoded (below)
//   2  R8G8B8A8_UNORM       r occlusion, g roughness, b metallic: glTF's ORM
//                           channel order, so a packed ORM texture maps 1:1
//   3  R16G16B16A16_SFLOAT  rgb emitted radiance; HDR, so not 8 bit
//   4  R32_UINT             entity id for picking and outlines, 0 where nothing
//                           was drawn. Not read by lighting_pass
//   D  D32_SFLOAT           depth; world position is rebuilt from it

// Octahedral normal encoding (Cigolle et al. 2014, "A Survey of Efficient
// Representations for Independent Unit Vectors"). Projects the unit sphere onto
// an octahedron and unfolds that into a square, so two channels hold a normal
// with the error spread evenly over all directions, where storing xy and
// rebuilding z loses the sign of z and precision near the horizon.

float2 OctWrap(float2 v) {
    // Not sign(): sign(0) is 0, which would collapse the fold's edges.
    float2 signs = float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
    return (1.0 - abs(v.yx)) * signs;
}

// n must be unit length. Result is in [-1, 1]^2.
float2 EncodeNormal(float3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    return n.z >= 0.0 ? n.xy : OctWrap(n.xy);
}

float3 DecodeNormal(float2 e) {
    float3 n = float3(e, 1.0 - abs(e.x) - abs(e.y));
    float t = saturate(-n.z);
    n.x += n.x >= 0.0 ? -t : t;
    n.y += n.y >= 0.0 ? -t : t;
    return normalize(n);
}
