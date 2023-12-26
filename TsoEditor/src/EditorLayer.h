#pragma once

#include "TSO.h"
#include "glm/glm.hpp"
#include "Tso/Core/TimeStep.h"
#include "Tso/Renderer/OrthographicCameraController.h"
#include "Tso/Renderer/Renderer2D.h"
#include "Tso/Renderer/FrameBuffer.h"
#include "Tso/Scene/Scene.h"


namespace Tso {

	class EditorLayer : public Layer {
        struct MovaData {
            glm::vec2 originPos;
            glm::vec2 targetPos;
            float startTime;

        };

    public:
        EditorLayer();
        ~EditorLayer() = default;

        virtual void OnImGuiRender() override;

        virtual void OnUpdate(TimeStep ts)override;

        virtual void OnEvent(Event& event)override;

    private:
        template<typename T>
        T LinearInterpretMove(const float& start, const float& duration, const float& timestamp, const T& pos, const T& target, bool& movable);

        bool OnMouseButton(MouseButtonPressedEvent& e);

        bool OnMouseMove(MouseMovedEvent& e);

    private:

        OrthographicCameraController m_CameraController;
        glm::vec3 m_TrianglePos;
        Ref<Shader> m_Shader;
        Ref<ShaderLibrary> m_ShaderLibrary;
        Ref<Texture2D> m_Texture, m_TileTexture;
        Ref<SubTexture2D> m_subTexture, m_sub1;
        Ref<FrameBuffer> m_FrameBuffer;
        Ref<Scene>      m_Scene;

        float m_MoveSpeed = 1.0f;
        bool m_LpMovable = false;
        float m_Time = 0.0;
        MovaData m_MoveData;

        float m_MouseX = 0.f, m_MouseY = 0.f;
        bool m_ViewportFocused = false;

        glm::vec2 m_ViewportSize = { 720.0 , 1280.0 };
};

}