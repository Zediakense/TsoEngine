#pragma once
#include "Scene.h"
#include <utility>
#include "Tso/Core/Log.h"
#include "glm/glm.hpp"


namespace Tso {

	class Entity {
		friend class SceneHierarchyPanel;
	public:
		Entity() {}
		Entity(const entt::entity& entityId , Scene* scene , const std::string& name = std::string());

		template<typename T , typename... Arg>
        T& AddComponent(Arg&& ...arg){
			return m_Scene->m_Registry.emplace<T>(m_EntityID, std::forward<Arg>(arg)...);
        }

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityID, std::forward<Args>(args)...);
			//m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			//TSO_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityID);
		}

		template<typename T>
		void RemoveComponent()
		{
			//TSO_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			if(HasComponent<T>())
				m_Scene->m_Registry.remove<T>(m_EntityID);
			else {
				TSO_CORE_WARN("no component to remove");
			}
		}
		

		template<typename T>
		bool HasComponent() {
			return m_Scene && m_Scene->m_Registry.all_of<T>(m_EntityID);
		}

		glm::mat4 GetWorldTransform();

		glm::mat4 GetParentTransform();


		std::unordered_map<uint64_t, Ref<Entity>> GetChildren() { return m_Scene->GetEntityChildren(*this); }

		//TODO: add a method to get a specified child @SuchanTso

		void AddChild(Entity& child);

		void SetParent(Entity& parent);

		Ref<Entity> GetParent() { return m_Scene->GetEntityParent(*this); }

		void RemoveChild(Entity& child);

		uint32_t GetEntityID() { return (uint32_t)m_EntityID; }
        
        uint64_t GetUUID();

		operator uint32_t() { return (uint32_t)m_EntityID; }
		bool operator==(Entity& other) {
			return other.m_EntityID != entt::null && other.m_EntityID == m_EntityID;
		}
        operator entt::entity() const { return m_EntityID; }

		operator bool() const { return this != nullptr; }

	private:

		Scene* m_Scene = nullptr;
		entt::entity m_EntityID = entt::null ;
        std::string m_Name = "";

	};


}
