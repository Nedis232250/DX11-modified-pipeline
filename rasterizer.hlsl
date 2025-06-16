RWBuffer<uint> screen_buf : register(u0);
RWBuffer<float> z_buf : register(u1);
StructuredBuffer<float> p : register(t0);
StructuredBuffer<float> c : register(t1);
StructuredBuffer<uint> misc : register(t2);
StructuredBuffer<float> tri_meta : register(t3);

[numthreads(1024, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID) {
    uint i = threadID.x;
    uint local = i % 144;
    uint t = i * rcp(144.0f);
    uint qx = local % 12;
    uint qy = local * rcp(12.0f);
    
    if (i < (misc[2] * 144)) {
        float2 v1 = float2(p[t * 9], p[t * 9 + 1]);
        float2 v2 = float2(p[t * 9 + 3], p[t * 9 + 4]);
        float2 v3 = float2(p[t * 9 + 6], p[t * 9 + 7]);
        float3 zs = float3(p[t * 9 + 2], p[t * 9 + 5], p[t * 9 + 8]);
        float3 c1 = float3(c[t * 13], c[t * 13 + 1], c[t * 13 + 2]);
        float3 c2 = float3(c[t * 13 + 4], c[t * 13 + 5], c[t * 13 + 6]);
        float3 c3 = float3(c[t * 13 + 8], c[t * 13 + 9], c[t * 13 + 10]);
        float3 alphas = float3(c[t * 13 + 3], c[t * 13 + 7], c[t * 13 + 11]);
        float l1, l2, l3, a;
        uint hit = 0;
        
        uint width = uint(tri_meta[t * 5 + 2]) - uint(tri_meta[t * 5]);
        uint height = uint(tri_meta[t * 5 + 3]) - uint(tri_meta[t * 5 + 1]);
        
        uint minx = uint(tri_meta[t * 5]) + floor((float(qx) * rcp(12.0f)) * float(width));
        uint miny = uint(tri_meta[t * 5 + 1]) + floor((float(qy) * rcp(12.0f)) * float(height));
        uint maxx = minx + ceil(float(width) * rcp(12.0f));
        uint maxy = miny + ceil(float(height) * rcp(12.0f));
        
        l1 = ((float(minx) - v2.x) * (v3.y - v2.y) - (float(miny) - v2.y) * (v3.x - v2.x)) * tri_meta[t * 5 + 4];
        l2 = ((float(minx) - v3.x) * (v1.y - v3.y) - (float(miny) - v3.y) * (v1.x - v3.x)) * tri_meta[t * 5 + 4];
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((float(maxx) - v2.x) * (v3.y - v2.y) - (float(miny) - v2.y) * (v3.x - v2.x)) * tri_meta[t * 5 + 4];
        l2 = ((float(maxx) - v3.x) * (v1.y - v3.y) - (float(miny) - v3.y) * (v1.x - v3.x)) * tri_meta[t * 5 + 4];
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((float(minx) - v2.x) * (v3.y - v2.y) - (float(maxy) - v2.y) * (v3.x - v2.x)) * tri_meta[t * 5 + 4];
        l2 = ((float(minx) - v3.x) * (v1.y - v3.y) - (float(maxy) - v3.y) * (v1.x - v3.x)) * tri_meta[t * 5 + 4];
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((float(maxx) - v2.x) * (v3.y - v2.y) - (float(maxy) - v2.y) * (v3.x - v2.x)) * tri_meta[t * 5 + 4];
        l2 = ((float(maxx) - v3.x) * (v1.y - v3.y) - (float(maxy) - v3.y) * (v1.x - v3.x)) * tri_meta[t * 5 + 4];
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        if (hit == 0)
            return;
        
        if (c[t * 13 + 12] == 1.0f) {
            for (uint x = minx; x < maxx; x++) {
                for (uint y = miny; y < maxy; y++) {
                    l1 = ((float(x) - v2.x) * (v3.y - v2.y) - (float(y) - v2.y) * (v3.x - v2.x)) * tri_meta[t * 5 + 4];
                    l2 = ((float(x) - v3.x) * (v1.y - v3.y) - (float(y) - v3.y) * (v1.x - v3.x)) * tri_meta[t * 5 + 4];
                    l3 = 1.0f - l2 - l1;
                
                    if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f && z_buf[1540 * y + x] > (l1 * zs.x + l2 * zs.y + l3 * zs.z)) {
                        a = l1 * alphas.r + l2 * alphas.g + l3 * alphas.b;
                        screen_buf[1540 * y + x] = uint(((clamp(l1 * c1.r + l2 * c2.r + l3 * c3.r, 0.0f, 1.0f) * a + (1 - a) * ((screen_buf[1540 * y + x] >> 16) & 0xFF) * rcp(255.0f))) * 255u) << 16 |
                                                   uint(((clamp(l1 * c1.g + l2 * c2.g + l3 * c3.g, 0.0f, 1.0f) * a + (1 - a) * ((screen_buf[1540 * y + x] >> 8) & 0xFF) * rcp(255.0f))) * 255u) << 8 |
                                                   uint(((clamp(l1 * c1.b + l2 * c2.b + l3 * c3.b, 0.0f, 1.0f) * a + (1 - a) * (screen_buf[1540 * y + x] & 0xFF) * rcp(255.0f))) * 255u);
                        z_buf[1540 * y + x] = (l1 * zs.x + l2 * zs.y + l3 * zs.z);
                    }
                }
            }
        }
    }
}
