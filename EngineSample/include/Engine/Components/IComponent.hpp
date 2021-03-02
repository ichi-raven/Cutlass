#pragma once

namespace Engine
{
    class IComponent
    {
    public:
        virtual void update() = 0;
        
        //どうしても識別したいときに使う
        void setID(int ID)
        {
            mID = ID;
        }
        
        int getID() const
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
        int mID;
        bool mUpdateFlag;
    };
}
