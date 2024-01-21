//
//  Font.cpp
//  TsoEngine
//
//  Created by SuchanTso on 2024/1/20.
//
#include "TPch.h"
#include "Font.h"

#ifdef TSO_PLATFORM_WINDOWS
#undef INFINITE
#endif






namespace Tso {

    

template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
    static Ref<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
        const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height)
    {
        msdf_atlas::GeneratorAttributes attributes;
        attributes.config.overlapSupport = true;
        attributes.scanlinePass = true;

        msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
        generator.setAttributes(attributes);
        generator.setThreadCount(8);
        generator.generate(glyphs.data(), (int)glyphs.size());

        msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

        TextureSpecification spec;
        spec.Width = bitmap.width;
        spec.Height = bitmap.height;
        spec.Format = ImageFormat::RGB8;
        spec.GenerateMips = false;

        Ref<Texture2D> texture = Texture2D::Create(spec);
        texture->SetData((void*)bitmap.pixels, bitmap.width * bitmap.height * 3);
        return texture;
    }



Font::Font(const std::filesystem::path& fontPath)
:m_Data(new MSDFData)
{
    msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype();
    TSO_CORE_ASSERT(ft , "Unable to init freetype");
    msdfgen::FontHandle *font = msdfgen::loadFont(ft, fontPath.string().c_str());
    if (!font) {
        TSO_CORE_ERROR("cannot open fontPath {0}" , fontPath.string().c_str());
    }
            
    struct CharsetRange
    {
        uint32_t Begin, End;
    };

    // From imgui_draw.cpp
    static const CharsetRange charsetRanges[] =
    {
        { 0x0020, 0x00FF }
    };
    msdf_atlas::Charset charset;
    for(CharsetRange range : charsetRanges){
        for(uint32_t c = range.Begin ; c <= range.End ; c++){
            charset.add(c);
        }
    }
    
    double fontScale = 1.0;
    m_Data->FontGeometry = msdf_atlas::FontGeometry(&m_Data->Glyphs);
    int glyphsLoaded = m_Data->FontGeometry.loadCharset(font, fontScale, charset);
    TSO_CORE_INFO("Loaded {} glyphs from font (out of {})", glyphsLoaded, charset.size());
    
    
    double emSize = 40.0;
    msdf_atlas::TightAtlasPacker atlasPacker;
    atlasPacker.setPixelRange(2.0);
    atlasPacker.setMiterLimit(1.0);
    
    atlasPacker.setPadding(0);
    atlasPacker.setScale(emSize);
    
    int remaining = atlasPacker.pack(m_Data->Glyphs.data(), (int)m_Data->Glyphs.size());//将拿到的字形放入纹理图集里，remain表示没放入的字形数
    TSO_CORE_ASSERT(remaining == 0);
    
    int width, height;
    atlasPacker.getDimensions(width, height);
    emSize = atlasPacker.getScale();

    
#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8
    
    uint64_t coloringSeed = 0;
    bool expensiveColoring = false;
    if (expensiveColoring)
    {
        msdf_atlas::Workload([&glyphs = m_Data->Glyphs, &coloringSeed](int i, int threadNo) -> bool {
            unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
            glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
            return true;
            }, m_Data->Glyphs.size()).finish(THREAD_COUNT);
    }
    else {
        unsigned long long glyphSeed = coloringSeed;
        for (msdf_atlas::GlyphGeometry& glyph : m_Data->Glyphs)
        {
            glyphSeed *= LCG_MULTIPLIER;
            glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
        }
    }
    
    m_AtlasTexture = CreateAndCacheAtlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>("Test", (float)emSize, m_Data->Glyphs, m_Data->FontGeometry, width, height);

    
    msdfgen::destroyFont(font);
    
    msdfgen::deinitializeFreetype(ft);
    
}

Font::~Font(){
    delete m_Data;
}

}
