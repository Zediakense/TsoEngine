#pragma once
#include "Tso/Renderer/Buffer.h"

namespace Tso {

	class OpenGLVertexBuffer : public VertexBuffer {
	public:

		OpenGLVertexBuffer(float* vertices, uint32_t size);
		OpenGLVertexBuffer(uint32_t size);


		virtual ~OpenGLVertexBuffer(){}

		virtual void Bind() const override;

		virtual void UnBind() const override;

		virtual void SetData(const void* data, uint32_t size) override;

		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		virtual const BufferLayout& GetLayout()const override { return m_Layout; }

	private:
		uint32_t m_RendererId;
		BufferLayout m_Layout;


	};

	class OpenGLIndexBuffer :public IndexBuffer {
	public:

		OpenGLIndexBuffer(uint32_t* indices, uint32_t count);

		virtual ~OpenGLIndexBuffer(){}

		virtual void Bind() const override;

		virtual void UnBind() const override;

		virtual uint32_t GetCount() override{ return m_Count; }

	private:
		uint32_t m_RendererId;
		uint32_t m_Count;


	};

}