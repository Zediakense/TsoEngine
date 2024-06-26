#pragma once
#include "RenderCommand.h"
#include "OrthographicCamera.h"
#include "Shader.h"


namespace Tso {


	class Renderer {
	public:

		static void BeginScene(OrthographicCamera& camera);
		static void EndScene();

		static void Submit(const Ref<VertexArray>& vertexArray , const Ref<Shader>& shader , const glm::mat4 transform = glm::mat4(1.0f));

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void OnWindowResize(uint32_t width , uint32_t height);

	private:

		struct SceneData {
			glm::mat4 ProjViewMatrix;
		};

		static SceneData* m_SceneData;
	};

}