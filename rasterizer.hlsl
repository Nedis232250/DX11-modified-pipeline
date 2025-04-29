RWBuffer<unsigned int> screen_buf : register(u0);
RWBuffer<float> z_buf : register(u1);
RWBuffer<unsigned int> positions : register(u2);
RWBuffer<float> colors : register(u3);
RWBuffer<unsigned int> misc : register(u4);
RWBuffer<unsigned int> texture_ : register(u5);

[numthreads(1024, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID) {
    unsigned int index = threadID.x + groupID.x * 1024;

    if (index < misc[2]) {
        unsigned int x1 = positions[0 + index * 9];
        unsigned int y1 = positions[1 + index * 9];
        unsigned int z1 = positions[2 + index * 9];
        unsigned int x2 = positions[3 + index * 9];
        unsigned int y2 = positions[4 + index * 9];
        unsigned int z2 = positions[5 + index * 9];
        unsigned int x3 = positions[6 + index * 9];
        unsigned int y3 = positions[7 + index * 9];
        unsigned int z3 = positions[8 + index * 9];
        unsigned int width = misc[0];
        unsigned int height = misc[1];
        float4 color1 = { colors[0 + index * 13], colors[1 + index * 13], colors[2 + index * 13], colors[3 + index * 13] };
        float4 color2 = { colors[4 + index * 13], colors[5 + index * 13], colors[6 + index * 13], colors[7 + index * 13] };
        float4 color3 = { colors[8 + index * 13], colors[9 + index * 13], colors[10 + index * 13], colors[11 + index * 13] };
        
        // 0.0 for textures, 1.0 for colors, and when textures color.z/color.b doesnt matter
        float mode = colors[12 + index * 13];

        x1 = clamp(x1, 0.0f, (float) (width - 1));
        y1 = clamp(y1, 0.0f, (float) (height - 1));
        x2 = clamp(x2, 0.0f, (float) (width - 1));
        y2 = clamp(y2, 0.0f, (float) (height - 1));
        x3 = clamp(x3, 0.0f, (float) (width - 1));
        y3 = clamp(y3, 0.0f, (float) (height - 1));

        float minx = (x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3));
        float maxx = (x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3));
        float miny = (y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3));
        float maxy = (y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3));
        
        for (unsigned int x = minx; x < maxx; x++) {
            for (unsigned int y = miny; y < maxy; y++) {
                float denom = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3);
                if (denom == 0) continue;

                float l1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / denom;
                float l2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / denom;
                float l3 = 1.0 - l1 - l2;

                if (l1 >= 0 && l2 >= 0 && l3 >= 0) {
                    float z_interpolated = clamp(l1 * z1 + l2 * z2 + l3 * z3, 0.0f, 255.0f);

                    if ((z_buf[y * width + x] > z_interpolated) && mode == 1.0f) {
                        float alpha = clamp(l1 * color1.a + l2 * color2.a + l3 * color3.a, 0.0f, 1.0f);
                        float r = clamp(l1 * color1.r + l2 * color2.r + l3 * color3.r, 0.0f, 1.0f) * alpha + ((screen_buf[y * width + x] >> 16 & 0xFF) / 255.0f) * (1.0 - alpha);
                        float g = clamp(l1 * color1.g + l2 * color2.g + l3 * color3.g, 0.0f, 1.0f) * alpha + ((screen_buf[y * width + x] >> 8 & 0xFF) / 255.0f) * (1.0 - alpha);
                        float b = clamp(l1 * color1.b + l2 * color2.b + l3 * color3.b, 0.0f, 1.0f) * alpha + ((screen_buf[y * width + x] & 0xFF) / 255.0f) * (1.0 - alpha);

                        screen_buf[y * width + x] = (int) (r * 255) << 16 | (int) (g * 255) << 8 | (int) (b * 255);
                        z_buf[y * width + x] = z_interpolated;
                    } else if ((z_buf[y * width + x] > z_interpolated) && mode == 0.0f) {
                        float u = l1 * color1.x + l2 * color2.x + l3 * color3.x;
                        float v = l1 * color1.y + l2 * color2.y + l3 * color3.y;
                        u = clamp(u, 0.0f, 1.0f);
                        v = clamp(v, 0.0f, 1.0f);
                        
                        unsigned int tx = clamp((uint)(u * texture_[0]), 0u, texture_[0] - 1u);
                        unsigned int ty = clamp((uint)(v * texture_[1]), 0u, texture_[1] - 1u);
                        unsigned int tex_color = texture_[2 + ty * texture_[0] + tx];
                        float alpha = clamp(l1 * color1.a + l2 * color2.a + l3 * color3.a, 0.0f, 1.0f);
                        float r = ((tex_color >> 16) & 0xFF) * alpha / 255.0f + ((screen_buf[y * width + x] >> 16 & 0xFF) / 255.0f) * (1.0 - alpha);
                        float g = ((tex_color >> 8) & 0xFF) * alpha / 255.0f + ((screen_buf[y * width + x] >> 8 & 0xFF) / 255.0f) * (1.0 - alpha);
                        float b = ((tex_color) & 0xFF) * alpha / 255.0f + ((screen_buf[y * width + x] & 0xFF) / 255.0f) * (1.0 - alpha);
                        
                        screen_buf[y * width + x] = (int) (r * 255) << 16 | (int) (g * 255) << 8 | (int) (b * 255);
                        z_buf[y * width + x] = z_interpolated;
                    }
                }
            }
        }
    }
}
