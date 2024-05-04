#include "TPch.h"
#include "Scene.h"
#include "Entity.h"
#include "Component.h"
#include "Tso/Renderer/Renderer2D.h"
#include "Tso/Scripting/ScriptingEngine.h"

#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"
#include "Tso/Renderer/Font.h"

#include "Tso/Core/Application.h"

namespace Utils {
    b2BodyType Rigidbody2DTypeToBox2DBody(Tso::Rigidbody2DComponent::BodyType bodyType)
    {
        switch (bodyType)
        {
        case Tso::Rigidbody2DComponent::BodyType::Static:    return b2_staticBody;
        case Tso::Rigidbody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
        case Tso::Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
        }

        TSO_CORE_ASSERT(false, "Unknown body type");
        return b2_staticBody;
    }
}


namespace Tso {

Scene::Scene(){
    
}


Entity Scene::CreateEntity(const std::string& name){
    return CreateEntityWithID(UUID() , name);
}

Entity Scene::CreateEntityWithID(const UUID& uuid , const std::string& name){
    entt::entity entityID = m_Registry.create();
    std::string entityName = name.length() > 0 ? name : ("blankEntity");
    Entity res = Entity(entityID , this , name);
    res.AddComponent<TransformComponent>(glm::vec3(0.0 , 0.0 , 0.9));
    res.AddComponent<IDComponent>(uuid);
    res.AddComponent<TagComponent>(entityName);
    res.AddComponent<ActiveComponent>();
    m_EntityMap[uuid] = (uint32_t)entityID;
    return res;
}

Entity Scene::GetEntityByUUID(const UUID& uuid)
{
    if (m_EntityMap.find((uint64_t)uuid) != m_EntityMap.end()) {
        return { (entt::entity)m_EntityMap.at(uuid), this };
    }
    else {
        TSO_CORE_ERROR("cannot find entity keeps uuid({}) in this scene", uuid);
    }
    return Entity();
}

bool Scene::EntityExist(const UUID& uuid)
{
    
    return m_EntityMap.find(uint64_t(uuid)) != m_EntityMap.end();
}

void Scene::OnUpdate(TimeStep ts)
{
    if (!m_Pause) {
        m_Time += ts.GetSecond();

        //update script

        m_Registry.view<ScriptComponent>().each([=](auto entity, auto& sc) {
            Entity e = { entity, this };
            ScriptingEngine::OnUpdateEntity(e, ts);
            });

        m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
            {
                // TODO: Move to Scene::OnScenePlay
                if (!nsc.Instance && nsc.hasBind)
                {
                    nsc.Instance = nsc.InstantiateScript();
                    nsc.Instance->m_Entity = Entity{ entity, this };
                    nsc.Instance->OnCreate();
                }
                if (nsc.hasBind)
                    nsc.Instance->OnUpdate(ts);
            });


        if (m_PhysicWorld) {
            const int32_t velocityIterations = 6;
            const int32_t positionIterations = 2;
            m_PhysicWorld->Step(ts, velocityIterations, positionIterations);

            // Retrieve transform from Box2D
            auto view = m_Registry.view<Rigidbody2DComponent>();
            for (auto e : view)
            {
                Entity entity = { e, this };
                auto& transform = entity.GetComponent<TransformComponent>();
                auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

                b2Body* body = (b2Body*)rb2d.RuntimeBody;

                const auto& position = body->GetPosition();

                glm::mat4 temTransform = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f));//TODO: set z as 0.0f may not correct
                temTransform = inverse(entity.GetParentTransform()) * temTransform;

                transform.m_Translation.x = temTransform[3][0];
                transform.m_Translation.y = temTransform[3][1];
                transform.m_Rotation.z = body->GetAngle();


            }
        }
    }

    //deal not active entity
    //delete them
    auto activeView = m_Registry.view<ActiveComponent>();
    for (auto& e : activeView) {
        Entity entity = { e, this };

        auto& activeC = entity.GetComponent<ActiveComponent>();
        if (!activeC.Active) {
            DeleteEntity(entity);
        }
    }


    auto view = m_Registry.view<TransformComponent, CameraComponent>();
    glm::mat4* mainCameraTransfrom = nullptr;

    for (auto& e : view) {
        auto& camera = view.get<CameraComponent>(e);
        if (camera.m_Pramiary) {
            mainCamera = &camera.m_Camera;
            auto& transfrom = view.get<TransformComponent>(e);
            mainCameraTransfrom = &transfrom.GetTransform();
        }
    }
 
    if (mainCamera) {
        Renderer2D::BeginScene(*mainCamera, *mainCameraTransfrom);
        
        auto group = m_Registry.view<Renderable , TransformComponent>();
        for (auto& entity : group) {
            const auto& [render, trans] = group.get<Renderable, TransformComponent>(entity);
            Entity tEntity = Entity(entity , this);
            glm::mat4 transform = tEntity.GetWorldTransform();
            if(render.type == RenderType::PureColor){
                Renderer2D::DrawQuad(transform,render.m_Color , (int)entity);
            }
            else{
                if(render.isSubtexture){
                    Renderer2D::DrawQuad(transform,render.subTexture , (int)entity);
                }
                else{
                    if(render.subTexture && render.subTexture->GetTexture())
                        Renderer2D::DrawQuad(transform,render.subTexture->GetTexture() , (int)entity);
                }
            }
        }
        
        auto textGroup = m_Registry.view<TransformComponent, TextComponent>();
        for(auto& e : textGroup){
            const auto& [transComp , textComp] = textGroup.get<TransformComponent, TextComponent>(e);
            if(textComp.TextFont && textComp.Text.length() > 0){
                Renderer2D::DrawString(textComp.TextFont, transComp.GetTransform(), textComp.Text , textComp.textParam , (int)e);
            }
        }
        
        Renderer2D::EndScene();
    }

    
}

b2Body* Scene::CreatePhysicBody(Entity& entity)
{
    auto& transform = entity.GetComponent<TransformComponent>();
    auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
    std::string tag = entity.GetComponent<TagComponent>().m_Name;

    b2BodyDef bodyDef;
    bodyDef.type = Utils::Rigidbody2DTypeToBox2DBody(rb2d.Type);
    auto localTransform = entity.GetWorldTransform();
    //glm::vec4 worldTranslation = localTransform * glm::vec4(transform.m_Translation, 1.0f);
    TSO_CORE_INFO("{} set position as {} , {}", tag, localTransform[3][0], localTransform[3][1]);
    bodyDef.position.Set(localTransform[3][0], localTransform[3][1]);
    bodyDef.angle = transform.m_Rotation.z;

    b2Body* body = m_PhysicWorld->CreateBody(&bodyDef);
    body->SetFixedRotation(rb2d.FixedRotation);
    rb2d.RuntimeBody = body;

    if (entity.HasComponent<BoxCollider2DComponent>())
    {
        auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

        b2PolygonShape boxShape;
        boxShape.SetAsBox(bc2d.Size.x * transform.m_Scale.x, bc2d.Size.y * transform.m_Scale.y, b2Vec2(bc2d.Offset.x, bc2d.Offset.y), 0.0f);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &boxShape;
        fixtureDef.density = bc2d.Density;
        fixtureDef.friction = bc2d.Friction;
        fixtureDef.restitution = bc2d.Restitution;
        fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
        body->CreateFixture(&fixtureDef);
        body->GetUserData().pointer = reinterpret_cast<uintptr_t>(&m_EntityMap[entity.GetUUID()]);

        

    }
    return body;
}

template<typename... Component>
static void CopyComponentIfExists(Entity dst, Entity src)
{
    ([&]()
        {
            if (src.HasComponent<Component>())
                dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
        }(), ...);
}

template<typename... Component>
static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
{
    CopyComponentIfExists<Component...>(dst, src);
}

Entity Scene::CopyEntity(Entity entity)
{
    std::string name = entity.GetComponent<TagComponent>().m_Name;
    Entity newEntity = CreateEntity(name);
    CopyComponentIfExists(AllComponents{}, newEntity, entity);
    if (entity.GetParent()) {
        newEntity.SetParent(*entity.GetParent());
    }
    return newEntity;
}

void Scene::SetEntityParent(Entity& parent , Entity& child)
{
    if (m_ChildrenMap[parent.GetUUID()].find(child.GetUUID()) != m_ChildrenMap[parent.GetUUID()].end()) {
        TSO_CORE_WARN("two entities are already parent nodes!!");
    }
    else {
        m_ChildrenMap[parent.GetUUID()][child.GetUUID()] = CreateRef<Entity>(child);
        m_ParentMap[child.GetUUID()] = CreateRef<Entity>(parent);
    }
}


void Scene::RemoveChild(Entity& parent, Entity& child)
{
    for (auto& grandChildren : child.GetChildren()) {
        RemoveChild(child, *grandChildren.second);
    }

    
        m_ChildrenMap[parent.GetUUID()].erase(child.GetUUID());
        m_ParentMap[child.GetUUID()] = nullptr;
    
    //DeleteEntity(child);

}

std::unordered_map<uint64_t, Ref<Entity>> Scene::GetEntityChildren(Entity& parent)
{
    return m_ChildrenMap[parent.GetUUID()];
}

Ref<Entity> Scene::GetEntityParent(Entity& child)
{
    return m_ParentMap[child.GetUUID()];
}

void Scene::DeleteEntity(Entity entity){
    TSO_CORE_ASSERT(m_EntityMap.find(entity.GetUUID()) != m_EntityMap.end(), "cannot find entity , maybe an invalid scene");
    if (entity.HasComponent<Rigidbody2DComponent>()) {
        auto& rigidc = entity.GetComponent<Rigidbody2DComponent>();
        b2Body* body = (b2Body*)rigidc.RuntimeBody;
        body->SetEnabled(false);
        m_PhysicWorld->DestroyBody(body);
        
    }
    m_EntityMap.erase(entity.GetUUID());
    m_Registry.destroy(entity);
    //ScriptingEngine::OnDeleteEntity(entity);

}

void Scene::OnScenePlay()
{
    ScriptingEngine::OnScenePlay(this);

    auto& sView = m_Registry.view<ScriptComponent>();
    for (auto e : sView) {
        Entity entity = { e , this };
        //std::string className = entity.GetComponent<ScriptComponent>().ClassName;
        ScriptingEngine::OnCreateEntity(entity);
    }

    m_PhysicWorld = new b2World({ 0.0f , -9.8f});
    m_PhysicsListener = new NativeContactListener(this);

    m_PhysicWorld->SetContactListener(m_PhysicsListener);

    //
    auto view = m_Registry.view<Rigidbody2DComponent>();
    for (auto e : view)
    {
        Entity entity = { e, this };
        CreatePhysicBody(entity);
    }
    m_Pause = false;
}

void Scene::OnSceneStop()
{
    if (m_PhysicWorld) {
        delete m_PhysicWorld;
        m_PhysicWorld = nullptr;
    }
    if (m_PhysicsListener) {
        delete m_PhysicsListener;
        m_PhysicsListener = nullptr;
    }
    ScriptingEngine::OnSceneStop();
    m_Pause = true;
}





}
