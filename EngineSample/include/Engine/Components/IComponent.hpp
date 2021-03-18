#pragma once

using uint32_t = unsigned int;

namespace Engine
{
    class IComponent
    {
    public:
        IComponent()
        : mUpdateFlag(true)
        {
            static uint32_t IDGen = 0;
            mID = IDGen++;
        }

        virtual ~IComponent(){};

        virtual void update() = 0;
        
        //どうしても識別したいときに使う        
        uint32_t getID() const
        {
            return mID;
        }

        //更新が不要な場合はセットする
        //普通は呼ばなくていい
        void setUpdateFlag(bool flag)
        {
            mUpdateFlag = flag;
        }

        void setUpdateFlag()
        {
            mUpdateFlag = !mUpdateFlag;
        }

        const bool getUpdateFlag() const
        {
            return mUpdateFlag;
        }

    private:
        uint32_t mID;
        bool mUpdateFlag;
    };
}
