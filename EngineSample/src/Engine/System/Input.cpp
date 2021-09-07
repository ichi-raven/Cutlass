#include <Engine/System/Input.hpp>

namespace Engine
{
    uint32_t Input::getKey(Cutlass::Key key)
    {
        return mContext->getKey(key);
    }

    bool Input::getKeyDown(Cutlass::Key key)
    {
        return mContext->getKey(key) == 1;
    }

    glm::vec2 Input::getMousePos()
    {
        double x, y;
        mContext->getMousePos(x, y);
        return glm::vec2(x, y);
    }
}
