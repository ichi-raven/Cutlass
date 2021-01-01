#pragma once

#include <typeinfo>
#include <memory>
#include <unordered_map>
#include <Cutlass.hpp>

#include "../Components/IComponent.hpp"


//アクタのクラスのヘッダにこれを書けばヘッダが自動生成できます
// #define GEN_ACTOR_CLASS(CLASSNAME, COMMONREGION_TYPE) \
// public:\
// virtual ~CLASSNAME() final override;\
// virtual void init([[maybe_unused]] Engine::ActorsInScene<COMMONREGION_TYPE>& actors, [[maybe_unused]] std::shared_ptr<COMMONREGION_TYPE> commonRegion) final override;\
// virtual void update([[maybe_unused]] Engine::ActorsInScene<COMMONREGION_TYPE>& actors, [[maybe_unused]] std::shared_ptr<COMMONREGION_TYPE> commonRegion) final override;\
// private:

#define GEN_ACTOR_CLASS(CLASSNAME, COMMONREGION_TYPE) \
public:\
CLASSNAME() = delete;\
CLASSNAME(Engine::ActorsInScene<COMMONREGION_TYPE>& actors, std::shared_ptr<COMMONREGION_TYPE> const sceneCommonRegion, std::shared_ptr<Cutlass::Context> const context, const std::vector<Cutlass::HWindow>& hwindows):IActor(actors, sceneCommonRegion, context, hwindows){}\
virtual ~CLASSNAME() final override;\
virtual void init() final override;\
virtual void update() final override;\
private:

//init, updateの引数自動生成(COMMONREGION_TYPE...共有領域の型, ARG_***...引数名)
// #define INIT_ARG_ACTOR(COMMONREGION_TYPE, ARG_ACTORS, ARG_COMMONREGION) [[maybe_unused]] Engine::ActorsInScene<COMMONREGION_TYPE>& ARG_ACTORS, [[maybe_unused]] std::shared_ptr<COMMONREGION_TYPE> ARG_COMMONREGION
// #define UPDATE_ARG_ACTOR(COMMONREGION_TYPE, ARG_ACTORS, ARG_COMMONREGION) [[maybe_unused]] Engine::ActorsInScene<COMMONREGION_TYPE>& ARG_ACTORS, [[maybe_unused]] std::shared_ptr<COMMONREGION_TYPE> ARG_COMMONREGION

//[[maybe_unused]]とは、その引数を関数内で使用しなかったとしてもコンパイラに警告を吐かせない属性です
//よくわからない場合は上記マクロを使用すればいいかも?

namespace Engine
{
    template<typename SceneCommonRegion>
    class ActorsInScene;
    
    template<typename SceneCommonRegion>
    class IActor
    {
    public:
        IActor
        (
            ActorsInScene<SceneCommonRegion>& actors,
            std::shared_ptr<SceneCommonRegion> const sceneCommonRegion,
            std::shared_ptr<Cutlass::Context> const context, 
            const std::vector<Cutlass::HWindow>& hwindows
        )
        : mActors(actors)
        , mCommonRegion(sceneCommonRegion)
        , mContext(context)
        , mHWindows(hwindows)
        {
            //これも大して影響ないとは思うけどチューニングしたら速いかも知れない
            mComponentsVec.reserve(5);
        }

        //Noncopyable, Nonmovable
        IActor(const IActor&) = delete;
        IActor &operator=(const IActor&) = delete;
        IActor(IActor&&) = delete;
        IActor &operator=(IActor&&) = delete;

        virtual ~IActor(){};

        virtual void init() = 0;
        virtual void update() = 0;
        // virtual void init([[maybe_unused]] ActorsInScene<SceneCommonRegion>& actors, [[maybe_unused]] std::shared_ptr<SceneCommonRegion> commonRegion) = 0;//注意 : 他アクタは取得できますが、望む情報が得られない可能性があります
        // virtual void update([[maybe_unused]] ActorsInScene<SceneCommonRegion>& actors, [[maybe_unused]] std::shared_ptr<SceneCommonRegion> commonRegion) = 0;

        void updateAll()
        {
            update();
            for (auto& component : mComponentsVec)
                component->update();
        }

        //なければnullopt, 同型のComponentのうち最も前のものを返します
        template<typename RequiredComponent>
        std::optional<std::shared_ptr<RequiredComponent>> getComponent() //なければ無効値、必ずチェックを(shared_ptrのoperator boolで判別可能)
        {
            const auto& iter = mComponents.find(typeid(RequiredComponent).hash_code());
            return (iter != mComponents.end()) ? std::make_optional(std::dynamic_pointer_cast<RequiredComponent>(iter->second[0])) : std::nullopt;
        }

        //なければnullopt, 同型のComponentを全て取得します
        template<typename RequiredComponent>
        std::optional<std::vector<std::shared_ptr<RequiredComponent>>>& getComponents() //なければ無効値、必ずチェックを(shared_ptrのoperator boolで判別可能)
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

        ActorsInScene<SceneCommonRegion>& getActors() const
        {
            return mActors;
        }

        std::shared_ptr<SceneCommonRegion> const getCommonRegion() const
		{
			return mCommonRegion;
		}

		std::shared_ptr<Cutlass::Context> const getContext() const
		{
			return mContext;
		}

		const std::vector<Cutlass::HWindow>& getHWindows() const
		{
			return mHWindows;
		}

    private:
        //キーは型のハッシュ値
        std::unordered_map<size_t, std::vector<std::shared_ptr<IComponent>>> mComponents;
        std::vector<std::shared_ptr<IComponent>> mComponentsVec;

        ActorsInScene<SceneCommonRegion>& mActors;
        std::shared_ptr<SceneCommonRegion> mCommonRegion;

        //Cutlass
        std::shared_ptr<Cutlass::Context> mContext;
        const std::vector<Cutlass::HWindow>& mHWindows;
    };
};