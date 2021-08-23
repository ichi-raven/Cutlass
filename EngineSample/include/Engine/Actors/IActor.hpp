#pragma once

#include <typeinfo>
#include <memory>
#include <unordered_map>
#include <Cutlass.hpp>
#include <iostream>

#include "../Components/IComponent.hpp"

//これをクラス宣言部に書けば、継承した関数はすべて定義されます
//使用例はSampleActor等を参照してください
#define GEN_ACTOR(ACTOR_TYPE, COMMONREGION_TYPE) \
public:\
ACTOR_TYPE(Engine::ActorsInScene<COMMONREGION_TYPE>& actors, const std::shared_ptr<COMMONREGION_TYPE>& sceneCommonRegion, const std::shared_ptr<Cutlass::Context>& context, const std::shared_ptr<Engine::System>& system):IActor(actors, sceneCommonRegion, context, system){}\
virtual ~ACTOR_TYPE() override;\
virtual void awake() override;\
virtual void init() override;\
virtual void update() override;\
private:

//コンストラクタとデストラクタのみVer.
#define GEN_ACTOR_CONSTRUCTOR_DESTRUCTOR(ACTOR_TYPE, COMMONREGION_TYPE) \
public:\
ACTOR_TYPE(Engine::ActorsInScene<COMMONREGION_TYPE>& actors, const std::shared_ptr<COMMONREGION_TYPE>& sceneCommonRegion, const std::shared_ptr<Cutlass::Context>& context, const std::shared_ptr<Engine::System>& system):IActor(actors, sceneCommonRegion, context, system){}\
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
            const std::shared_ptr<CommonRegion>& sceneCommonRegion,
            const std::shared_ptr<Cutlass::Context>& context, 
            const std::shared_ptr<System>& system
        )
        : mActors(actors)
        , mCommonRegion(sceneCommonRegion)
        , mContext(context)
        , mSystem(system)
        {
            mComponentsVec.reserve(4);
        }

        //Noncopyable, Nonmovable
        IActor(const IActor&) = delete;
        IActor& operator=(const IActor&) = delete;
        IActor(IActor&&) = delete;
        IActor& operator=(IActor&&) = delete;

        virtual ~IActor(){};

        //構築
        virtual void awake() = 0;

        //初期化(他の参照等をとる)
        virtual void init() = 0;
        
        virtual void update() = 0;

        void updateAll()
        {
            update();
            for (auto& component : mComponentsVec)
                if(component->getUpdateFlag())
                    component->update();
        }

        //なければnullopt, 同型のComponentのうち最も前のものを返します
        template<typename RequiredComponent>
        std::optional<std::shared_ptr<RequiredComponent>> getComponent()
        {
            const auto& iter = mComponents.find(typeid(RequiredComponent).hash_code());
            return (iter != mComponents.end()) ? std::make_optional(std::dynamic_pointer_cast<RequiredComponent>(iter->second[0])) : std::nullopt;
        }

        //なければnullopt, 同型のComponentを全て取得します(ちょっと重い)
        template<typename RequiredComponent>
        std::optional<std::vector<std::shared_ptr<RequiredComponent>>> getComponents()
        {
            const auto& iter = mComponents.find(typeid(RequiredComponent).hash_code());
            if (iter != mComponents.end())
            {   
                std::vector<std::shared_ptr<RequiredComponent>> rtn;
                rtn.resize(iter->second.size());
                for(size_t i = 0; i < rtn.size(); ++i)
                    rtn[i] = std::dynamic_pointer_cast<RequiredComponent>(iter->second[i]);
                
                return std::make_optional(rtn);
            }
            else
                return std::nullopt;
        }

    protected:

        template<typename Component>
        std::shared_ptr<Component> addComponent()
        {
            auto tmp = std::make_shared<Component>();
            //tmp->setContext(mContext);

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

        template<typename Actor>
		std::shared_ptr<Actor> addActor(const std::string_view actorName, bool autoInit = false)
		{
            mActors.template addActor<Actor>(actorName, autoInit);
        }

        template<typename RequiredActor>
	    std::optional<std::shared_ptr<RequiredActor>> getActor(const std::string_view actorName)
        {
            return mActors.template getActor<RequiredActor>(actorName);
        }

        void removeActor(const std::string_view actorName)
		{
			mActors.removeActor(actorName);
		}

        const std::shared_ptr<CommonRegion>& getCommonRegion() const
		{
			return mCommonRegion;
		}

		const std::shared_ptr<Cutlass::Context>& getContext() const
		{
			return mContext;
		}

		const std::shared_ptr<System>& getSystem() const
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