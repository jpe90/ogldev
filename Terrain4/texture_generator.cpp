#include <stdio.h>
#include <stdlib.h>

#include "texture_generator.h"
#include "terrain.h"
#include "ogldev_stb_image.h"

TextureGenerator::TextureGenerator()
{
}

void TextureGenerator::LoadTile(const char* pFilename)
{
    if (m_numTextureTiles == MAX_TEXTURE_TILES) {
        printf("%s:%d: exceeded the maximum of texture tiles with '%s'\n", __FILE__, __LINE__, pFilename);
        exit(0);
    }

    m_textureTiles[m_numTextureTiles].Image.Load(pFilename);
    m_numTextureTiles++;
}


Texture* TextureGenerator::GenerateTexture(int TextureSize, BaseTerrain* pTerrain)
{
    if (m_numTextureTiles == 0) {
        printf("%s:%d: no texture tiles loaded\n", __FILE__, __LINE__);
        exit(0);
    }

    CalculateTextureRegions();

    int BPP = 3;
    int TextureBytes = TextureSize * TextureSize * BPP;
    unsigned char* pTextureData = (unsigned char*)malloc(TextureBytes);
    unsigned char* p = pTextureData;

    float HeightMapToTextureRatio = (float)pTerrain->GetSize() / (float)TextureSize;

    printf("Height map to texture ratio: %f\n", HeightMapToTextureRatio);

    for (int y = 0 ; y < TextureSize ; y++) {
        for (int x = 0 ; x < TextureSize ; x++) {

            float InterpolatedHeight = pTerrain->GetHeightInterpolated((float)x * HeightMapToTextureRatio, (float)y * HeightMapToTextureRatio);

            float Red = 0.0f;
            float Green = 0.0f;
            float Blue = 0.0f;

            for (int Tile = 0 ; Tile < m_numTextureTiles ; Tile++) {
                Vector3f Color = m_textureTiles[Tile].Image.GetColor(x, y);

                float BlendFactor = RegionPercent(Tile, InterpolatedHeight);

                Red   += BlendFactor * Color.r;
                Green += BlendFactor * Color.g;
                Blue  += BlendFactor * Color.b;
            }

            if (Red > 255.0f || Green > 255.0f || Blue > 255.0f) {
                printf("%d:%d: %f %f %f\n", y, x, Red, Green, Blue);
                exit(0);
            }

            p[0] = (unsigned char)Red;
            p[1] = (unsigned char)Green;
            p[2] = (unsigned char)Blue;

            p += 3;
        }
    }

    Texture* pTexture = new Texture(GL_TEXTURE_2D);

    pTexture->LoadRaw(TextureSize, TextureSize, BPP, pTextureData);

    free(pTextureData);

    return pTexture;
}


void TextureGenerator::CalculateTextureRegions()
{
    int RangePerTile = 255 / m_numTextureTiles;
    int Remainder = 255 - RangePerTile * m_numTextureTiles;

    if (Remainder < 0) {
        printf("%s:%d: negative remainder %d (num tiles %d range per tile %d)\n", __FILE__, __LINE__, Remainder, m_numTextureTiles, RangePerTile);
        exit(0);
    }

    float LastHeight = -1.0f;

    for (int i = 0 ; i < m_numTextureTiles ; i++) {
        m_textureTiles[i].HeightDesc.Low = LastHeight + 1;
        LastHeight += RangePerTile;
        m_textureTiles[i].HeightDesc.Optimal = LastHeight;
        m_textureTiles[i].HeightDesc.High = LastHeight - m_textureTiles[i].HeightDesc.Low + LastHeight;

        m_textureTiles[i].HeightDesc.Print(); printf("\n");
    }
}


float TextureGenerator::RegionPercent(int Tile, float Height)
{
    float Percent = 0.0f;

    if (Height < m_textureTiles[Tile].HeightDesc.Low) {
        Percent = 0.0f;
    } else if (Height > m_textureTiles[Tile].HeightDesc.High) {
        Percent = 0.0f;
    } else if (Height < m_textureTiles[Tile].HeightDesc.Optimal) {
        float Nom = (float)Height - (float)m_textureTiles[Tile].HeightDesc.Low;
        float Denom = (float)m_textureTiles[Tile].HeightDesc.Optimal - (float)m_textureTiles[Tile].HeightDesc.Low;
        Percent = Nom / Denom;
    } else if (Height > m_textureTiles[Tile].HeightDesc.Optimal) {
        float Nom = (float)m_textureTiles[Tile].HeightDesc.High - (float)Height;
        float Denom = (float)m_textureTiles[Tile].HeightDesc.High - (float)m_textureTiles[Tile].HeightDesc.Optimal;
        Percent = Nom / Denom;
    } else {
        Percent = 1.0f;
    }

    if ((Percent < 0.0f) || (Percent > 1.0f)) {
        printf("%s:%d - Invalid percent %f\n", __FILE__, __LINE__, Percent);
        exit(0);
    }

    return Percent;
}