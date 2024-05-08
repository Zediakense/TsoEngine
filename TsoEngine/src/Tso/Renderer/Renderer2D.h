//
//  Renderer2D.hpp
//  TsoEngine
//
//  Created by SuchanTso on 2023/9/14.
//

#ifndef Renderer2D_hpp
#define Renderer2D_hpp

#include <stdio.h>
#include "OrthographicCamera.h"
#include "glm/glm.hpp"
#include "Shader.h"
#include "Texture.h"
#include "SubTexture2D.h"
#include "Tso/Scene/SceneCamera.h"

namespace Tso {
class Font;
struct TextParam;

struct RendererSpec {
    std::filesystem::path shaderPath;
    BufferLayout layout;
};

class Renderer2D{
public:
    static void Init();
    
    static void BeginScene(const OrthographicCamera& camera);

    static void BeginScene(const SceneCamera& camera, const glm::mat4& cameraTransform);
    
    static void EndScene();

    static void Flush();
    
    static void DrawQuad(const glm::vec2& position , const glm::vec2& scale , const glm::vec4& color , const int& entityID);
    static void DrawQuad(const glm::vec3& position , const glm::vec2& scale , const glm::vec4& color , const int& entityID);
    static void DrawQuad(const glm::vec3& position , const float& rotation , const glm::vec2& scale , const glm::vec4& color , const int& entityID);
    
    static void DrawQuad(const glm::mat4& transform , const glm::vec4& color , const int& entityID);
    static void DrawString(const Ref<Font> font , const glm::mat4& transform , const std::string& text , const TextParam& textParam , const int& entityID);
    
    static void DrawQuad(const glm::vec2& position , const glm::vec2& scale , Ref<Texture2D> texture , const int& entityID);
    static void DrawQuad(const glm::vec3& position , const glm::vec2& scale , Ref<Texture2D> texture , const int& entityID);
    static void DrawQuad(const glm::vec3& position , const float& rotation , const glm::vec2& scale , Ref<Texture2D> texture , const int& entityID);
    static void DrawQuad(const glm::vec3& position , const float& rotation , const glm::vec2& scale , Ref<SubTexture2D> subTexture , const int& entityID);
    static void DrawQuad(const glm::mat4& transform , Ref<Texture2D> texture , const int& entityID);
    static void DrawQuad(const glm::mat4& transform , Ref<SubTexture2D> subTexture , const int& entityID);


    struct Statistics {
        uint32_t DrawCalls = 0;
        uint32_t QuadCount = 0;

        uint32_t GetTotalVertexCount() { return QuadCount * 4; }
        uint32_t GetTotalIndexCount() { return QuadCount * 6; }

    };
    static void ResetStat();
    static Statistics GetStat();

private:
    static Ref<IndexBuffer> GenIndexBuffer(const uint32_t& maxIndexCounts);

    template<typename T>
    static void InitRenderer(Ref<Shader>& shader , Ref<VertexArray>& vertexArray , Ref<VertexBuffer>& vertexBuffer , T** vertexBase ,Ref<IndexBuffer>& indexBuffer,const RendererSpec& rendererSpec);
    static void GenDefaultTexture();
private:
    static void FlushAndRest();
};

}

#endif /* Renderer2D_hpp */
