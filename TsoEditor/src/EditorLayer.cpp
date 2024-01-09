#include "TPch.h"
#include "EditorLayer.h"
#include "imgui.h"
#include "Tso/Scene/Scene.h"
#include "Tso/Scene/Entity.h"
#include "Tso/Scene/Component.h"
#include "glm/gtc/type_ptr.hpp"
#include "Tso/Scene/Entity.h"
#include "Tso/Scene/Seriealizer.h"


namespace Tso {
    EditorLayer::EditorLayer()
        :Layer("EditorLayer"),
        m_CameraController(1280.0 / 720, true),
        m_TrianglePos(glm::vec3(0.f))
    {
        m_ShaderLibrary = std::make_shared<ShaderLibrary>();

        m_Shader = m_ShaderLibrary->Load("asset/shader/Shader2D.glsl");
        Renderer2D::Init(m_Shader);
        m_Scene = std::make_shared<Scene>();
        m_Panel.SetContext(m_Scene);

        std::string lp = "asset/lp2.png";

        std::string tileMap = "asset/tilemap_packed.png";


        m_Texture = Texture2D::Create(lp);
        m_TileTexture = Texture2D::Create(tileMap);
        m_subTexture = SubTexture2D::CreateByCoord(m_TileTexture, { 16.0 , 16.0 }, { 2.0 , 3.0 }, { 1.0 , 1.0 });
        m_sub1 = SubTexture2D::CreateByCoord(m_TileTexture, { 16.0 , 16.0 }, { 6.0 , 0.0 }, { 1.0 , 1.0 });

        FrameBufferInfo info = { (uint32_t)m_ViewportSize.x , (uint32_t)m_ViewportSize.y , false };
        m_FrameBuffer = FrameBuffer::Create(info);

        m_TrianglePos = glm::vec3(0.0, 0.0, 0.1);

        m_MoveData.startTime = m_Time;
        m_MoveData.targetPos = glm::vec2(0.0, 0.0);
        m_MoveData.originPos = glm::vec2(0.0, 0.0);

        

        /*auto e = m_Scene->CreateEntity("EEEE");

        e.AddComponent<NativeScriptComponent>().Bind<CircleBehavior>();*/

        auto t = m_Scene->CreateEntity("Static Quad");

        m_CameraEntity = m_Scene->CreateEntity("MainCamera");
//        m_CameraEntity.RemoveComponent<Renderable>();
        auto& camera = m_CameraEntity.AddComponent<CameraComponent>();
        camera.m_Pramiary = true;
        m_CameraEntity.AddComponent<NativeScriptComponent>().Bind<Controlable>();


    }


    void EditorLayer::OnImGuiRender()
    {


        //    static bool show = true;
        //    ImGui::ShowDemoWindow(&show);

            // If you strip some features of, this demo is pretty much equivalent to calling DockSpaceOverViewport()!
            // In most cases you should be able to just call DockSpaceOverViewport() and ignore all the code below!
            // In this specific demo, we are not using DockSpaceOverViewport() because:
            // - we allow the host window to be floating/moveable instead of filling the viewport (when opt_fullscreen == false)
            // - we allow the host window to have padding (when opt_padding == true)
            // - we have a local menu bar in the host window (vs. you could use BeginMainMenuBar() + DockSpaceOverViewport() in your code!)
            // TL;DR; this demo is more complicated than what you would normally use.
            // If we removed all the options we are showcasing, this demo would become:
            //     void ShowExampleAppDockSpace()
            //     {
            //         ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
            //     }

        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static bool dockSpaceOpen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockSpaceOpen, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        else
        {
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Editor"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
                ImGui::MenuItem("Padding", NULL, &opt_padding);
                ImGui::Separator();

                ImGui::Separator();

                if (ImGui::MenuItem("Save", NULL, false)) {
                    Seriealizer seriealizer(m_Scene.get());
                    seriealizer.SeriealizeScene("asset/testScene.tso");
                }

                if (ImGui::MenuItem("Close", NULL, false)){
                    dockSpaceOpen = false;
                    Application::Get().OnClose();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
{

            ImGui::Begin("RenderInfo");

            ImGui::Text("Render2DInfo");

            auto stat = Renderer2D::GetStat();

            ImGui::Text("DrawCalls : %d ", stat.DrawCalls);
            ImGui::Text("QuadsCount : %d ", stat.QuadCount);
            ImGui::Text("QuadVertices : %d", stat.GetTotalVertexCount());
            ImGui::Text("QuadIndices : %d", stat.GetTotalIndexCount());

            ImGui::DragFloat3("ComeraPos", glm::value_ptr(m_CameraController.GetCamera().GetPosition()));

            ImGui::End();

            ImGui::Begin("Viewport");
            m_ViewportFocused = ImGui::IsWindowFocused();
            Application::Get().GetGUILayer()->BlockEvents(!m_ViewportFocused);

            auto content = ImGui::GetContentRegionAvail();
            if (content.x > 0.f && content.y > 0.f && (content.x != m_ViewportSize.x || content.y != m_ViewportSize.y)) {
                m_ViewportSize = { content.x , content.y };
                m_FrameBuffer->Resize(uint32_t(m_ViewportSize.x), uint32_t(m_ViewportSize.y));
                auto& camera = m_CameraEntity.GetComponent<CameraComponent>();
                if(!camera.FixedAspectRatio)
                camera.m_Camera.SetViewportSize(m_ViewportSize.x , m_ViewportSize.y);
                        //m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
            }
            uint32_t fbId = m_FrameBuffer->GetColorAttachment();

            ImGui::Image((void*)fbId, ImVec2{ m_ViewportSize.x , m_ViewportSize.y },ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

            ImGui::End();
}
        m_Panel.OnGuiRender();
        ImGui::End();
    }


    template<typename T>
    T EditorLayer::LinearInterpretMove(const float& start, const float& duration, const float& timestamp, const T& pos, const T& target, bool& movable) {
        if (timestamp > start + duration) {
            movable = false;
            return target;
        }
        if (!movable) {
            return pos;
        }
        float ratio = (timestamp - start) / duration;
        T res = (target - pos) * ratio + pos;
        //    TSO_INFO("moving , start from {0} , duration is {1} , time = {2}" , start , duration , timestamp);

        return res;
    }


    void EditorLayer::OnUpdate(TimeStep ts)
    {

        Renderer2D::ResetStat();
        if(m_ViewportFocused){
            m_CameraController.OnUpdate(ts);
        }

        m_FrameBuffer->Bind();
//        Renderer2D::BeginScene(m_CameraController.GetCamera());
        RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
        RenderCommand::Clear();
        m_Scene->OnUpdate(ts);
        


//        for (float x = -5.0f; x < 5.0f; x += 0.5f) {
//            for (float y = -5.0f; y < 5.0f; y += 0.5f) {
//                glm::vec4 color = { (x + 5.0) / 10.0 ,0.4 , (y + 5.0) / 10.0 ,1.0 };
//                Renderer2D::DrawQuad({ x , y , 0.8f}, { 0.45 , 0.45 }, color);
//            }
//        }
        //Renderer2D::DrawQuad({ 0.f , 0.f , 0.8f }, { 0.45f , 0.45f }, { 0.8f , 0.3f , 0.2f , 1.f });


        Renderer2D::EndScene();
        m_FrameBuffer->UnBind();

        m_Time += ts.GetSecond();

    }

    void EditorLayer::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::OnMouseButton));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(EditorLayer::OnMouseMove));
        m_CameraController.OnEvent(event);

    }

    bool EditorLayer::OnMouseButton(MouseButtonPressedEvent& e) {

        if (e.GetButton() == TSO_MOUSE_BUTTON_1) {

            float width = 1280.f, height = 720.f;

            float asp = width / height;

            float targetX = m_MouseX / width * asp * 4 - 2 * asp;
            float targetY = m_MouseY / height * asp * 2 - asp;
            //TSO_INFO("mouse position = ({0} , {1}) , and caled pos = ({2} , {3})", m_MouseX, m_MouseY, targetX, targetY);


            m_MoveData.startTime = m_Time;
            m_MoveData.targetPos = glm::vec2(targetX, -targetY);
            m_MoveData.originPos = m_TrianglePos;
            m_LpMovable = true;
            return true;
        }
        return false;
    }

    bool EditorLayer::OnMouseMove(MouseMovedEvent& e) {


        m_MouseX = e.GetX();
        m_MouseY = e.GetY();

        return false;
    }

}
