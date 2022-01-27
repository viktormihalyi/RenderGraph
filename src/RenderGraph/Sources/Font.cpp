#if 0
#include "Font.hpp"

#pragma warning(push, 0)
#ifdef INFINITE // WinBase.h 
#undef INFINITE
#endif
#include "msdfgen/msdfgen-ext.h"
#include "msdfgen/msdfgen.h"
#pragma warning(pop)

#include "Utils/Assert.hpp"
#include "VulkanWrapper/Utils/ImageData.hpp"

#include <cstring>
#include <functional>
#include <map>


namespace RG {

static msdfgen::FontHandle* GetFont (const std::filesystem::path& fontFile)
{
    static msdfgen::FreetypeHandle*                    ft = nullptr;
    static std::map<std::string, msdfgen::FontHandle*> loadedFonts;

    if (ft == nullptr) {
        ft = msdfgen::initializeFreetype ();
    }
    if (RG_ERROR (ft == nullptr)) {
        throw std::runtime_error ("failed to initialize freetype");
    }

    const std::string file = fontFile.string ();

    if (loadedFonts.find (file) != loadedFonts.end ()) {
        return loadedFonts[file];
    }

    msdfgen::FontHandle* font = msdfgen::loadFont (ft, fontFile.string ().c_str ());
    if (RG_ERROR (font == nullptr)) {
        throw std::runtime_error ("failed to load font");
    }

    loadedFonts[fontFile.string ()] = font;

    return font;
}


struct Font::Impl {
    msdfgen::FontHandle* fontHandle;

    Impl (const std::filesystem::path& fontFile)
        : fontHandle (GetFont (fontFile))
    {
    }
};


Font::Font (const std::filesystem::path& fontFile)
    : impl (std::make_unique<Font::Impl> (fontFile))
    , fontFile (fontFile)
{
    msdfgen::FontMetrics met;
    msdfgen::getFontMetrics (met, impl->fontHandle);

    emSize             = met.emSize;
    ascenderY          = met.ascenderY;
    descenderY         = met.descenderY;
    lineHeight         = met.lineHeight;
    underlineY         = met.underlineY;
    underlineThickness = met.underlineThickness;
}


Font::~Font ()
{
}


static msdfgen::Shape LoadShape (msdfgen::FontHandle* font, uint32_t unicode)
{
    if (RG_ERROR (font == nullptr)) {
        throw std::runtime_error ("failed to load font");
    }

    msdfgen::Shape shape;
    RG_ASSERT (msdfgen::loadGlyph (shape, font, unicode));

    shape.inverseYAxis = true;
    shape.normalize ();

    msdfgen::edgeColoringSimple (shape, 3.0);

    RG_ASSERT (shape.validate ());

    // shape.inverseYAxis = true;

    return shape;
}


void Font::GetFontWhitespaceWidth (double& spaceAdvance, double& tabAdvance) const
{
    const bool success = msdfgen::getFontWhitespaceWidth (spaceAdvance, tabAdvance, impl->fontHandle);
    RG_ASSERT (success);
}


double Font::GetKerning (uint32_t unicode1, uint32_t unicode2) const
{
    double result = 0.0;

    const bool success = msdfgen::getKerning (result, impl->fontHandle, unicode1, unicode2);
    RG_ASSERT (success);

    return result;
}


template<typename T, size_t N>
static std::vector<T> BitmapToVector (const msdfgen::Bitmap<T, N>& bitmap)
{
    std::vector<T> result;

    const size_t elemCount = bitmap.width () * bitmap.height () * N;

    result.resize (elemCount);

    memcpy (result.data (), static_cast<const T*> (bitmap), elemCount * sizeof (float));

    return result;
}


/*
// from https://github.com/Chlumsky/msdfgen/blob/master/main.cpp#L893
static void GetAutoFrame (const uint32_t emSize, const msdfgen::Shape::Bounds& bounds, uint32_t width, uint32_t height,
                          msdfgen::Vector2& scale, msdfgen::Vector2& translate, msdfgen::Vector2& aspectRatio, msdfgen::Vector2& actScale)
{
    double l = bounds.l;
    double b = bounds.b;
    double r = bounds.r;
    double t = bounds.t;

    constexpr double range = 1.0;

    msdfgen::Vector2 frame (width, height);

    l -= .5 * range;
    b -= .5 * range;
    r += .5 * range;
    t += .5 * range;

    if (l >= r || b >= t)
        l = 0, b = 0, r = 1, t = 1;

    const msdfgen::Vector2 dims (r - l, t - b);

    const msdfgen::Vector2 actdims (bounds.r - bounds.l, bounds.t - bounds.b);

    if (actdims.x < actdims.y) {
        aspectRatio = { actdims.x / actdims.y, 1.f };
    } else {
        aspectRatio = { 1.f, actdims.y / actdims.x };
    }
    actScale = actdims / static_cast<float> (emSize);

    if (dims.x * frame.y < dims.y * frame.x) {
        translate.set (.5 * (frame.x / frame.y * dims.y - dims.x) - l, -b);
        scale = frame.y / dims.y;
    } else {
        translate.set (-l, .5 * (frame.y / frame.x * dims.x - dims.y) - b);
        scale = frame.x / dims.x;
    }
}
*/


template<size_t Components>
using DistanceMapGeneratorFunc = std::function<void (
    const msdfgen::BitmapRef<float, Components>& output,
    const msdfgen::Shape&                        shape,
    double                                       range,
    const msdfgen::Vector2&                      scale,
    const msdfgen::Vector2&                      translate)>;


template<size_t Components>
static GlyphData GetGlyph (msdfgen::FontHandle*                 fontHandle,
                           const uint32_t                       width,
                           const uint32_t                       height,
                           const uint32_t                       emSize,
                           const uint32_t                       unicode,
                           DistanceMapGeneratorFunc<Components> bitmapGeneratorFunc)
{
    msdfgen::Shape shape = LoadShape (fontHandle, unicode);

    msdfgen::Vector2 translate (0.0, 0.0);
    msdfgen::Vector2 scale (1.0, 1.0);
    msdfgen::Vector2 asp (0.5, 0.5);
    msdfgen::Vector2 actScale (0.5, 0.5);
    //GetAutoFrame (emSize, shape.getBounds (), width, height, scale, translate, asp, actScale);

    msdfgen::Bitmap<float, Components> bitmapData (width, height);

    static const double range = 4.0;

    bitmapGeneratorFunc (bitmapData, shape, range, scale, translate);

    GlyphData result;
    result.scale       = glm::vec2 { actScale.x, actScale.y };
    result.translation = glm::vec2 { 0, -shape.getBounds ().b } / static_cast<float> (emSize);
    result.data        = BitmapToVector<float, Components> (bitmapData);
    result.width       = width;
    result.height      = height;
    result.aspectRatio = glm::vec2 { asp.x, asp.y };

    RG::ImageData::FromDataFloat (result.data, width, height, Components).SaveTo (std::filesystem::current_path () / "temp" / (std::to_string (unicode) + ".png"));

    return result;
}


GlyphData Font::GetGlyphSDF (uint32_t width, uint32_t height, uint32_t unicode) const
{
    auto generator = [] (const msdfgen::BitmapRef<float, 1>& output,
                         const msdfgen::Shape&               shape,
                         double                              range,
                         const msdfgen::Vector2&             scale,
                         const msdfgen::Vector2&             translate) {
        msdfgen::generateSDF (output, shape, msdfgen::Projection { scale, translate }, range);
    };

    return GetGlyph<1> (impl->fontHandle, width, height, static_cast<uint32_t> (emSize), unicode, generator);
}


GlyphData Font::GetGlyphMDF (uint32_t width, uint32_t height, uint32_t unicode) const
{
    auto generator = [] (const msdfgen::BitmapRef<float, 3>& output,
                         const msdfgen::Shape&               shape,
                         double                              range,
                         const msdfgen::Vector2&             scale,
                         const msdfgen::Vector2&             translate) {
        msdfgen::generateMSDF (output, shape, range, scale, translate);
    };

    return GetGlyph<3> (impl->fontHandle, width, height, static_cast<uint32_t> (emSize), unicode, generator);
}


GlyphData Font::GetGlyphMTDF (uint32_t width, uint32_t height, uint32_t unicode) const
{
    auto generator = [] (const msdfgen::BitmapRef<float, 4>& output,
                         const msdfgen::Shape&               shape,
                         double                              range,
                         const msdfgen::Vector2&             scale,
                         const msdfgen::Vector2&             translate) {
        msdfgen::generateMTSDF (output, shape, range, scale, translate);
    };

    return GetGlyph<4> (impl->fontHandle, width, height, static_cast<uint32_t> (emSize), unicode, generator);
}

}
#endif