#pragma once

namespace Engine
{
    class IComponent
    {
    public:
        virtual void update() = 0;
        
        //どうしても識別したいときに使ってください
        void setID(int ID)
        {
            mID = ID;
        }
        
        int getID() const
        {
            return mID;
        }
    private:
        int mID;
    };
}
