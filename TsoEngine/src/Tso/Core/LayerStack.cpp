//
//  LayerStack.cpp
//  TsoEngine
//
//  Created by user on 2023/4/11.
//
#include "TPch.h"
#include "LayerStack.h"
namespace Tso{

LayerStack::LayerStack(){
    m_LayerInsertIndex = 0;
}

LayerStack::~LayerStack(){
    for(auto layer : m_Layers){
        delete layer;
    }
}


void LayerStack::PushLayer(Layer *layer){
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
    m_LayerInsertIndex++;
    layer->OnAttach();
}

void LayerStack::PushOverlay(Layer *overlay){
    m_Layers.emplace_back(overlay);
    overlay->OnAttach();
}

void LayerStack::PopLayer(Layer *layer){
    auto it = std::find(begin(),end(),layer);
    if(it != end()){
        //layer->OnDetach();
        m_Layers.erase(it);
        m_LayerInsertIndex--;
    }
}

void LayerStack::PopOverlay(Layer *overlay){
    auto it = std::find(begin(),end(),overlay);
    if(it != end()){
        //layer->OnDetach();
        m_Layers.erase(it);
    }
}
//overlay means little stuff we want to render in the front of the window
//it can even be understood as we keep two vector of layer stack but deal them in one

//the closer to the end of the stack , the more front to render,and the more fast to deal event

}
