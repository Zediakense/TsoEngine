//
//  LayerStack.hpp
//  TsoEngine
//
//  Created by user on 2023/4/11.
//
#include "Core.h"
#include "Layer.h"

namespace Tso{

class TSO_API LayerStack {
    
public:
    LayerStack();
    ~LayerStack();
    
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);
    
    std::vector<Layer*>::iterator begin(){return m_Layers.begin();}
    std::vector<Layer*>::iterator end(){return m_Layers.end();}

    
    
private:
    std::vector<Layer*> m_Layers;
    unsigned int m_LayerInsertIndex = 0;
    
};

}
