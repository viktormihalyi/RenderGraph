#if 0
#include "TestEnvironment.hpp"

#include "RenderGraph/Font.hpp"
#include "VulkanWrapper/Utils/ImageData.hpp"
#include "RenderGraph/UniformReflection.hpp"
#include "RenderGraph/VulkanEnvironment.hpp"
#include "RenderGraph/Window/Window.hpp"

using FontRenderingTests = HiddenWindowTestEnvironment;

#pragma warning(push, 0)
#pragma error(push, 0)

#define STB_DEFINE
#include "deprecated/stb.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#pragma error(pop)
#pragma warning(pop)

// used both to compute SDF and in 'shader'
float sdf_size         = 64.0; // the larger this is, the better large font sizes look
float pixel_dist_scale = 64.0; // trades off precision w/ ability to handle *smaller* sizes
int   onedge_value     = 128;
int   padding          = 3; // not used in shader

struct fontchar {
    float          advance;
    signed char    xoff;
    signed char    yoff;
    unsigned char  w;
    unsigned char  h;
    unsigned char *data;
};

std::vector<fontchar> fdata;

#define BITMAP_W 1200
#define BITMAP_H 800
unsigned char bitmap[BITMAP_H][BITMAP_W][3];

const char *sample       = "This is goofy text, size %d!";
const char *small_sample = "This is goofy text, size %d! Really needs in-shader supersampling to look good.";

void blend_pixel (int x, int y, int color, float alpha)
{
    int i;
    for (i = 0; i < 3; ++i)
        bitmap[y][x][i] = (unsigned char)(stb_lerp (alpha, bitmap[y][x][i], color) + 0.5); // round
}

void draw_char (float px, float py, char c, float relative_scale)
{
    int       x, y;
    fontchar *fc  = &fdata[c];
    float     fx0 = px + fc->xoff * relative_scale;
    float     fy0 = py + fc->yoff * relative_scale;
    float     fx1 = fx0 + fc->w * relative_scale;
    float     fy1 = fy0 + fc->h * relative_scale;
    int       ix0 = (int)floor (fx0);
    int       iy0 = (int)floor (fy0);
    int       ix1 = (int)ceil (fx1);
    int       iy1 = (int)ceil (fy1);
    // clamp to viewport
    if (ix0 < 0)
        ix0 = 0;
    if (iy0 < 0)
        iy0 = 0;
    if (ix1 > BITMAP_W)
        ix1 = BITMAP_W;
    if (iy1 > BITMAP_H)
        iy1 = BITMAP_H;

    for (y = iy0; y < iy1; ++y) {
        for (x = ix0; x < ix1; ++x) {
            float sdf_dist, pix_dist;
            float bmx = stb_linear_remap (x, fx0, fx1, 0, fc->w);
            float bmy = stb_linear_remap (y, fy0, fy1, 0, fc->h);
            int   v00, v01, v10, v11;
            float v0, v1, v;
            int   sx0 = (int)bmx;
            int   sx1 = sx0 + 1;
            int   sy0 = (int)bmy;
            int   sy1 = sy0 + 1;
            // compute lerp weights
            bmx = bmx - sx0;
            bmy = bmy - sy0;
            // clamp to edge
            sx0 = stb_clamp (sx0, 0, fc->w - 1);
            sx1 = stb_clamp (sx1, 0, fc->w - 1);
            sy0 = stb_clamp (sy0, 0, fc->h - 1);
            sy1 = stb_clamp (sy1, 0, fc->h - 1);
            // bilinear texture sample
            v00 = fc->data[sy0 * fc->w + sx0];
            v01 = fc->data[sy0 * fc->w + sx1];
            v10 = fc->data[sy1 * fc->w + sx0];
            v11 = fc->data[sy1 * fc->w + sx1];
            v0  = stb_lerp (bmx, v00, v01);
            v1  = stb_lerp (bmx, v10, v11);
            v   = stb_lerp (bmy, v0, v1);
#if 0
         // non-anti-aliased
         if (v > onedge_value)
            blend_pixel(x,y,0,1.0);
#else
            // Following math can be greatly simplified

            // convert distance in SDF value to distance in SDF bitmap
            sdf_dist = stb_linear_remap (v, onedge_value, onedge_value + pixel_dist_scale, 0, 1);
            // convert distance in SDF bitmap to distance in output bitmap
            pix_dist = sdf_dist * relative_scale;
            // anti-alias by mapping 1/2 pixel around contour from 0..1 alpha
            v = stb_linear_remap (pix_dist, -0.5f, 0.5f, 0, 1);
            if (v > 1)
                v = 1;
            if (v > 0)
                blend_pixel (x, y, 0, v);
#endif
        }
    }
}


void print_text (float x, float y, char *text, float scale)
{
    int i;
    for (i = 0; text[i]; ++i) {
        if (fdata[text[i]].data)
            draw_char (x, y, text[i], scale);
        x += fdata[text[i]].advance * scale;
    }
}


TEST_F (FontRenderingTests, DISABLED_SDF_a)
{
    int            ch;
    float          scale, ypos;
    stbtt_fontinfo font;
    const std::string    fileName = "c:/windows/fonts/times.ttf";
    void *         data     = stb_file (const_cast<char*> (fileName.c_str ()), NULL);
    stbtt_InitFont (&font, static_cast<unsigned char*> (data), 0);

    scale = stbtt_ScaleForPixelHeight (&font, sdf_size);

    fdata.resize (10000);

    for (ch = 32; ch < 9201; ++ch) {
        fontchar fc;
        int      xoff, yoff, w, h, advance;
        fc.data = stbtt_GetCodepointSDF (&font, scale, ch, padding, onedge_value, pixel_dist_scale, &w, &h, &xoff, &yoff);
        fc.xoff = xoff;
        fc.yoff = yoff;
        fc.w    = w;
        fc.h    = h;
        stbtt_GetCodepointHMetrics (&font, ch, &advance, NULL);
        fc.advance = advance * scale;
        fdata[ch]  = fc;
    }

    std::vector<uint8_t> dataV;
    const fontchar &     selected = fdata[9200];

    dataV.resize (selected.w * selected.h);
    memcpy (dataV.data (), selected.data, dataV.size ());
    GVK::ImageData asd = GVK::ImageData::FromDataUint (dataV, selected.w, selected.h, 1);
    asd.SaveTo (std::filesystem::current_path () / "test.png");

    ypos = 60;
    memset (bitmap, 255, sizeof (bitmap));
    print_text (400, ypos + 30, stb_sprintf ("sdf bitmap height %d", (int)sdf_size), 30 / sdf_size);
    ypos += 80;
    for (scale = 8.0; scale < 120.0; scale *= 1.33f) {
        print_text (80, ypos + scale, stb_sprintf (scale == 8.0 ? small_sample : sample, (int)scale), scale / sdf_size);
        ypos += scale * 1.05f + 20;
    }

    stbi_write_png ("sdf_test.png", BITMAP_W, BITMAP_H, 3, bitmap, 0);
}


void CompareImages2 (const std::string &name, const GVK::ImageData &referenceImage, const GVK::ImageData &actualImage)
{
    const bool isSame = referenceImage == actualImage;

    EXPECT_TRUE (isSame);

    if (!isSame) {
        actualImage.SaveTo (std::filesystem::current_path () / "temp" / name);
    }
}

#if 0

TEST_F (FontRenderingTests, MSDFGEN)
{
    DeviceExtra& device        = GetDeviceExtra ();
    Queue&       graphicsQueue = GetGraphicsQueue ();

    RG::GraphSettings s (device, 3);
    RG::RenderGraph   graph;

    constexpr uint glyphWidthHeight = 16;

    auto sp = std::make_unique<ShaderPipeline> (device);

    sp->SetVertexShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 2) in vec2 glyphPos;       // instanced
layout (location = 3) in uint glyphIndex;     // instanced

layout (location = 0) out vec2 textureCoords;
layout (location = 1) out uint glyphIdx;

struct SingleGlyphData {
    vec2 translation;
    vec2 scale;
    vec2 aspectRatio;
};

layout (binding = 1) uniform GlyphData {
    SingleGlyphData glyph[512];
    uint            fontSizePx;
};

layout (binding = 2) uniform Screen {
    uint screenWidth;
    uint screenHeight;
};


void main ()
{
    const vec2 scaling = vec2 (fontSizePx, fontSizePx) / (vec2 (screenWidth, screenHeight) / 2.f);
    const vec2 vertexPos = (position * glyph[glyphIndex].scale + glyph[glyphIndex].translation) * scaling;
    gl_Position = vec4 (vertexPos + glyphPos, 0.0, 1.0);
    textureCoords = 0.5f + glyph[glyphIndex].aspectRatio / 2.f * (uv * 2.f - 1.f);
    glyphIdx = glyphIndex;
}
    )");

    // draw second glyph
    sp->SetFragmentShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 textureCoords;
layout (location = 1) flat in uint glyphIndex;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2DArray sampl;


float GetMedian (float r, float g, float b) {
    return max (min (r, g), min (max (r, g), b));
}

float GetOpacity (sampler2DArray glyphArray, vec2 textureCoordinates, uint glyphIdx)
{
    const vec2 pos       = textureCoordinates.xy;
    const vec3 sampled   = texture (sampl, vec3 (textureCoordinates, glyphIdx)).rgb;
    const ivec2 sz       = textureSize (sampl, 0).xy;
    const float dx       = dFdx (pos.x) * sz.x; 
    const float dy       = dFdy (pos.y) * sz.y;
    const float toPixels = 8.0 * inversesqrt (dx * dx + dy * dy);
    const float sigDist  = GetMedian (sampled.r, sampled.g, sampled.b);
    const float w        = fwidth (sigDist);
    const float opacity  = smoothstep (0.5 - w, 0.5 + w, sigDist);
    return opacity;
}

void main ()
{
    const float dist = texture (sampl, vec3 (textureCoords.x, textureCoords.y, glyphIndex)).r;
    if (dist > 0.5f) {
        outColor = vec4 (vec3 (1), 1.f);
    } else {
        outColor = vec4 (vec3 (0), 1.f);
    }
    const float opacity = smoothstep (0.4, 0.6, dist);
    outColor = vec4 (vec3 (0), 1.f - opacity);

    // outColor = vec4 (vec3 (0), 0.9f - GetOpacity (sampl, textureCoords, glyphIndex));
}
    )");

    struct Vert {
        glm::vec2 position;
        glm::vec2 uv;
    };

    auto vbb = std::make_unique<VertexBufferTransferable<Vert>> (device, 512, std::vector<VkFormat> { ShaderTypes::Vec2f, ShaderTypes::Vec2f }, VK_VERTEX_INPUT_RATE_VERTEX);

    *vbb = std::vector<Vert> {
        { glm::vec2 (0.0f, 0.0f), glm::vec2 (0.f, 0.f) },
        { glm::vec2 (0.0f, -1.0f), glm::vec2 (0.f, 1.f) },
        { glm::vec2 (1.0f, -1.0f), glm::vec2 (1.f, 1.f) },
        { glm::vec2 (1.0f, 0.0f), glm::vec2 (1.f, 0.f) },
    };
    vbb->Flush ();

    IndexBufferTransferable ib (device, 6);
    ib.data = { 0, 1, 2, 0, 3, 2 };
    ib.Flush ();

    struct InstVert {
        glm::vec2 glyphPos;
        uint  glyphIndex;
    };

    FontManager fm ("C:\\Windows\\Fonts\\arialbd.ttf", glyphWidthHeight, glyphWidthHeight, FontManager::Type::SDF);

    auto instanceBuffer = std::make_unique<VertexBufferTransferable<InstVert>> (device, 512, std::vector<VkFormat> { ShaderTypes::Vec2f, ShaderTypes::Uint }, VK_VERTEX_INPUT_RATE_INSTANCE);

    VertexBufferList vbs;
    vbs.Add (vbb);
    vbs.Add (instanceBuffer);

    std::shared_ptr<RG::RenderOperation> renderOp = graph.CreateOperation<RG::RenderOperation> (std::make_unique<DrawableInfo> (5, 4, vbs, 6, ib.buffer.GetBufferToBind ()), sp);

    RG::WritableImageResourceP outputImage = graph.CreateResource<RG::WritableImageResource> (512, 512);
    RG::ReadOnlyImageResourceP glyphs      = graph.CreateResource<RG::ReadOnlyImageResource> (VK_FORMAT_R32_SFLOAT, glyphWidthHeight, glyphWidthHeight, 1, 512);

    graph.CreateInputConnection (*renderOp, *glyphs, std::make_unique<RG::ImageInputBinding> (0, *glyphs));
    graph.CreateOutputConnection (*renderOp, 0, *outputImage);

    RG::UniformReflection refl (graph);

    refl[renderOp][ShaderKind::Vertex]["GlyphData"]["fontSizePx"] = static_cast<uint> (24);

    fm.glyphLoaded += [&] (uint32_T unicode) {
        GlyphData gdata                                                                  = fm.GetGlyph (unicode);
        refl[renderOp][ShaderKind::Vertex]["GlyphData"]["glyph"][unicode]["translation"] = gdata.translation;
        refl[renderOp][ShaderKind::Vertex]["GlyphData"]["glyph"][unicode]["scale"]       = gdata.scale;
        refl[renderOp][ShaderKind::Vertex]["GlyphData"]["glyph"][unicode]["aspectRatio"] = gdata.aspectRatio;
    };

    refl[renderOp][ShaderKind::Vertex]["Screen"]["screenWidth"]  = GetWindow ().GetWidth ();
    refl[renderOp][ShaderKind::Vertex]["Screen"]["screenHeight"] = GetWindow ().GetHeight ();
    GetWindow ().events.resized += [&] (uint32_t newWidth, uint32_t newHeight) {
        refl[renderOp][ShaderKind::Vertex]["Screen"]["screenWidth"]  = newWidth;
        refl[renderOp][ShaderKind::Vertex]["Screen"]["screenHeight"] = newHeight;
    };

    graph.Compile (s);

    *instanceBuffer = {
        { glm::vec2 (-0.3, 0.0), 'g' },
        { glm::vec2 (+0.0, 0.0), 'T' },
        { glm::vec2 (+0.4, 0.0), 'e' },
        { glm::vec2 (+0.8, 0.0), 'h' },
        { glm::vec2 (+0.6, 0.0), 'l' },
    };


    instanceBuffer->Flush ();


    for (uint32_t unicode = 'A'; unicode < 'Z'; ++unicode) {
        glyphs->CopyLayer (fm.GetGlyph (unicode).data, unicode);
    }
    for (uint32_t unicode = 'a'; unicode < 'z'; ++unicode) {
        glyphs->CopyLayer (fm.GetGlyph (unicode).data, unicode);
    }
    glyphs->CopyLayer (fm.GetGlyph ('!').data, '!');

    refl.Flush (0);
    graph.Submit (0);

    vkQueueWaitIdle (graphicsQueue);
    vkDeviceWaitIdle (device);

    CompareImages2 ("G.png", ImageData (ReferenceImagesFolder / "G.png"), ImageData (GetDeviceExtra (), *outputImage->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
}

#endif
#endif