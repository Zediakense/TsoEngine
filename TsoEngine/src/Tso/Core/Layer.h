//
//  Layer.hpp
//  TsoEngine
//
//  Created by user on 2023/4/11.
//
#pragma once
#include "Core.h"
#include "Tso/Event/Event.h"
namespace Tso {

class TSO_API Layer{
    
public:
    Layer(const std::string& name = "Layer");
    
    virtual ~Layer() = default;
    
    virtual void OnAttach(){}
    virtual void OnDetach(){}
    virtual void OnUpdate(TimeStep ts){}
    virtual void OnEvent(Event& e){}
    virtual void OnImGuiRender() {}
    
    inline std::string GetName(){ return m_DebugName; }
    
    
protected:
    std::string m_DebugName;
};

}
