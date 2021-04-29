#pragma once

using uint32_t = unsigned int;

#include <memory>

namespace Cutlass
{
    class Context;
}

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
        
        void setContext(const std::shared_ptr<Cutlass::Context>& context)
        {
            mContext = context;
        }

        const std::shared_ptr<Cutlass::Context>& getContext()
        {
            return mContext;
        }

        //どうしても識別したいときに使う        
        uint32_t getID() const
        {
            return mID;
        }

        //更新が不要な場合はセットする
        //普通は呼ばなくていい
        void setUpdateFlag(const bool flag)
        {
            mUpdateFlag = flag;
        }

        const bool getUpdateFlag() const
        {
            return mUpdateFlag;
        }


    private:
        uint32_t mID;
        bool mUpdateFlag;
        std::shared_ptr<Cutlass::Context> mContext;

    };
}
