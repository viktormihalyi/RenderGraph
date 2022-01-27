#ifndef GEARSVK_FONT_HPP
#define GEARSVK_FONT_HPP

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/Event.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"
#include <memory>

#include <glm/glm.hpp>
#include <iostream>


inline std::ostream& operator<< (std::ostream& os, const glm::vec2& vec)
{
    os << "("
       << vec.x << ", "
       << vec.y << ", "
       << ")";
    return os;
}


inline std::ostream& operator<< (std::ostream& os, const glm::vec3& vec)
{
    os << "("
       << vec.x << ", "
       << vec.y << ", "
       << vec.z << ", "
       << ")";
    return os;
}


namespace RG {

struct GlyphData {
    std::vector<float> data;
    uint32_t           width  = 0;
    uint32_t           height = 0;
    glm::vec2          translation { 0.f, 0.f };
    glm::vec2          scale { 0.f, 0.f };
    glm::vec2          aspectRatio { 0.f, 0.f };
};


class RENDERGRAPH_DLL_EXPORT Font : public Noncopyable {
public:
    // hide font handle
    struct Impl;
    std::unique_ptr<Impl> impl;

    const std::filesystem::path fontFile;

    double emSize;
    double ascenderY, descenderY;
    double lineHeight;
    double underlineY, underlineThickness;

public:
    Font (const std::filesystem::path& fontFile);
    ~Font ();

    // usese msdfgen::generateSDF to generate a single channel bitmap
    GlyphData GetGlyphSDF (uint32_t width, uint32_t height, uint32_t unicode) const;

    // usese msdfgen::generateMDF to generate a multi (3) channel bitmap
    GlyphData GetGlyphMDF (uint32_t width, uint32_t height, uint32_t unicode) const;

    // usese msdfgen::generateMTDF to generate a multi (4) channel bitmap
    GlyphData GetGlyphMTDF (uint32_t width, uint32_t height, uint32_t unicode) const;

    void   GetFontWhitespaceWidth (double& spaceAdvance, double& tabAdvance) const;
    double GetKerning (uint32_t unicode1, uint32_t unicode2) const;
};


class FontManager {
public:
    enum class Type {
        SDF,
        MDF,
        MTDF
    };

    struct GlyphMetrics {
        std::vector<glm::vec2> translation;
        std::vector<glm::vec2> scale;
        std::vector<glm::vec2> aspectRatio;

        GlyphMetrics ()
        {
            translation.resize (512);
            scale.resize (512);
            aspectRatio.resize (512);
        }
    };

private:
    Font           font;
    const uint32_t width;
    const uint32_t height;
    const Type     distanceFieldType;

    std::unordered_map<uint32_t, std::unique_ptr<GlyphData>> loadedGlyphs;

public:
    RG::Event<uint32_t> glyphLoaded;

private:
    const GlyphData& Retrieve (const uint32_t unicode)
    {
        const auto it = loadedGlyphs.find (unicode);

        if (it == loadedGlyphs.end ()) {
            GlyphData data;

            if (distanceFieldType == Type::SDF) {
                data = font.GetGlyphSDF (width, height, unicode);
            } else if (distanceFieldType == Type::MDF) {
                data = font.GetGlyphMDF (width, height, unicode);
            } else if (distanceFieldType == Type::MTDF) {
                data = font.GetGlyphMTDF (width, height, unicode);
            } else {
                RG_BREAK ();
            }

            const auto insertResult = loadedGlyphs.insert ({ unicode, std::make_unique<GlyphData> (data) });

            RG_ASSERT (insertResult.second);

            glyphLoaded (unicode);

            return *insertResult.first->second;
        }

        return *it->second;
    }


public:
    FontManager (const std::filesystem::path& fontFile, const uint32_t width, const uint32_t height, const Type distanceFieldType)
        : font (fontFile)
        , width (width)
        , height (height)
        , distanceFieldType (distanceFieldType)
    {
    }

    const GlyphData& GetGlyph (const uint32_t unicode)
    {
        return Retrieve (unicode);
    }

    double GetKerning (uint32_t unicode1, uint32_t unicode2) const
    {
        return font.GetKerning (unicode1, unicode2);
    }
};

} // namespace RG


#endif
