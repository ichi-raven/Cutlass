#pragma once

#include <typeinfo>
#include <memory>
#include <unordered_map>
#include <Cutlass.hpp>

#include "../Components/IComponent.hpp"

//これをクラス宣言部に書けば、継承した関数はすべて定義されます
//使用例はSampleActor等を参照してください
#define GEN_ACTOR(ACTOR_TYPE, COMMONREGION_TYPE) \
public:\
ACTOR_TYPE(Engine::ActorsInScene<COMMONREGION_TYPE>& actors, std::shared_ptr<COMMONREGION_TYPE> const sceneCommonRegion, std::shared_ptr<Cutlass::Context> const context, std::shared_ptr<Engine::System> system):IActor(actors, sceneCommonRegion, context, system){}\
virtual ~ACTOR_TYPE() override;\
virtual void init() override;\
virtual void update() override;\
private:

//コンストラクタとデストラクタのみVer.
#define GEN_ACTOR_CONSTRUCTOR_DESTRUCTOR(ACTOR_TYPE, COMMONREGION_TYPE) \
public:\
ACTOR_TYPE(Engine::ActorsInScene<COMMONREGION_TYPE>& actors, std::shared_ptr<COMMONREGION_TYPE> const sceneCommonRegion, std::shared_ptr<Cutlass::Context> const context, std::shared_ptr<Engine::System> system):IActor(actors, sceneCommonRegion, context, system){}\
virtual ~ACTOR_TYPE() override;\
private:


namespace Engine
{
    class System;

    template<typename CommonRegion>
    class ActorsInScene;
    
    template<typename CommonRegion>
    class IActor
    {
    public:

        IActor() = delete;
        
        IActor
        (
            ActorsInScene<CommonRegion>& actors,
            std::shared_ptr<CommonRegion> const sceneCommonRegion,
            std::shared_ptr<Cutlass::Context> const context, 
            std::shared_ptr<System> const system
        )
        : mActors(actors)
        , mCommonRegion(sceneCommonRegion)
        , mContext(context)
        , mSystem(system)
        {
            //これも大して影響ないとは思うけどチューニングしたら速いかも知れない
            mComponentsVec.reserve(5);
        }

        //Noncopyable, Nonmovable
        IActor(const IActor&) = delete;
        IActor& operator=(const IActor&) = delete;
        IActor(IActor&&) = delete;
        IActor& operator=(IActor&&) = delete;

        virtual ~IActor(){};

        virtual void init() = 0;
        //lateinitは、Scene::initで相互に参照を取るActorを作成するときに使用します
        //initなどでどうしようもないときは使ってください
        virtual void lateinit(){}
        
        virtual void update() = 0;

        void updateAll()
        {
            update();
            for (auto& component : mComponentsVec)
                component->update();
        }

        //なければnullopt, 同型のComponentのうち最も前のものを返します
        template<typename RequiredComponent>
        std::optional<std::shared_ptr<RequiredComponent>> getComponent()
        {
            const auto& iter = mComponents.find(typeid(RequiredComponent).hash_code());
            return (iter != mComponents.end()) ? std::make_optional(std::dynamic_pointer_cast<RequiredComponent>(iter->second[0])) : std::nullopt;
        }

        //なければnullopt, 同型のComponentを全て取得します
        template<typename RequiredComponent>
        std::optional<std::vector<std::shared_ptr<RequiredComponent>>>& getComponents()
        {
            const auto& iter = mComponents.find(typeid(RequiredComponent).hash_code());
            return (iter != mComponents.end()) ? std::make_optional(std::dynamic_pointer_cast<RequiredComponent>(iter->second)) : std::nullopt;
        }

    protected:

        template<typename Component>
        std::shared_ptr<Component> addComponent()
        {
            auto tmp = std::make_shared<Component>();
            auto&& p = mComponents.emplace(typeid(Component).hash_code(), std::vector<std::shared_ptr<IComponent>>());
            if(p.second)
                p.first->second.emplace_back(tmp);
            else
                mComponents.at(typeid(Component).hash_code()).emplace_back(tmp);
            
            mComponentsVec.emplace_back(tmp);
            return tmp;
        }

        //引数付きコンストラクタもOK
        template<typename Component, typename... Args>
        std::shared_ptr<Component> addComponent(Args... constructArgs)
        {
            auto tmp = std::make_shared<Component>(constructArgs...);
            auto&& p = mComponents.emplace(typeid(Component).hash_code(), std::vector<std::shared_ptr<IComponent>>());
            if(p.second)
                p.first->second.emplace_back(tmp);
            else
                mComponents.at(typeid(Component).hash_code()).emplace_back(tmp);
            mComponentsVec.emplace_back(tmp);
            return tmp;
        }

        ActorsInScene<CommonRegion>& getActors() const
        {
            return mActors;
        }

        std::shared_ptr<CommonRegion> const getCommonRegion() const
		{
			return mCommonRegion;
		}

		std::shared_ptr<Cutlass::Context> const getContext() const
		{
			return mContext;
		}

		std::shared_ptr<System> const getSystem() const
        {
            return mSystem;
        }

    private:
        //キーは型のハッシュ値
        std::unordered_map<size_t, std::vector<std::shared_ptr<IComponent>>> mComponents;
        std::vector<std::shared_ptr<IComponent>> mComponentsVec;

        ActorsInScene<CommonRegion>& mActors;
        std::shared_ptr<CommonRegion> mCommonRegion;

        std::shared_ptr<System> mSystem;
       
        //Cutlass
        std::shared_ptr<Cutlass::Context> mContext;
    };
};