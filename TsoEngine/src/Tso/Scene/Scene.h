#pragma once
#include "entt.hpp"
#include "Tso/Core/TimeStep.h"
#include "Tso/Physics/CollideListener.h"

class b2World;

namespace Tso {
	class Entity;
	class Scene {
		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class Seriealizer;
	public:
		Scene() = default;
		~Scene() {}

		Entity CreateEntity(const std::string& name = "");
        
        void DeleteEntity(Entity entity);

		void OnScenePlay();

		void OnSceneStop();

		void OnUpdate(TimeStep ts);

		

	private:
		entt::registry m_Registry;

		float m_Time = 0.f;

		uint32_t m_EntityCount = 0;

		b2World* m_PhysicWorld = nullptr;

		NativeContactListener* m_PhysicsListener = nullptr;

		bool m_Pause = true;

	};



}
