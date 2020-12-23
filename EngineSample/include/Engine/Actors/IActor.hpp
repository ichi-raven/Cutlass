#pragma once

#include <typeinfo>
#include <memory>
#include <unordered_map>
#include <Cutlass/Cutlass.hpp>

#include "../Components/IComponent.hpp"


//アクタのクラスのヘッダにこれを書けばヘッダが自動生成できます
#define GEN_ACTOR_CLASS(CLASSNAME, COMMONREGION_TYPE) \
public:\
virtual ~CLASSNAME() final override;\
virtual void init([[maybe_unused]] Engine::ActorsInScene<COMMONREGION_TYPE>& actors, [[maybe_unused]] std::shared_ptr<COMMONREGION_TYPE> commonRegion) final override;\
virtual void update([[maybe_unused]] Engine::ActorsInScene<COMMONREGION_TYPE>& actors, [[maybe_unused]] std::shared_ptr<COMMONREGION_TYPE> commonRegion) final override;\
private:

//init, updateの引数自動生成(COMMONREGION_TYPE...共有領域の型, ARG_***...引数名)
#define INIT_ARG_ACTORS(COMMONREGION_TYPE, ARG_ACTORS, ARG_COMMONREGION) [[maybe_unused]] Engine::ActorsInScene<COMMONREGION_TYPE>& ARG_ACTORS, [[maybe_unused]] std::shared_ptr<COMMONREGION_TYPE> ARG_COMMONREGION
#define UPDATE_ARG_ACTORS(COMMONREGION_TYPE, ARG_ACTORS, ARG_COMMONREGION) [[maybe_unused]] Engine::ActorsInScene<COMMONREGION_TYPE>& ARG_ACTORS, [[maybe_unused]] std::shared_ptr<COMMONREGION_TYPE> ARG_COMMONREGION

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
        IActor()
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

        virtual void init([[maybe_unused]] ActorsInScene<SceneCommonRegion>& actors, [[maybe_unused]] std::shared_ptr<SceneCommonRegion> commonRegion) = 0;//注意 : 他アクタは取得できますが、望む情報が得られない可能性があります

        virtual void update([[maybe_unused]] ActorsInScene<SceneCommonRegion>& actors, [[maybe_unused]] std::shared_ptr<SceneCommonRegion> commonRegion) = 0;

        void updateAll([[maybe_unused]] ActorsInScene<SceneCommonRegion>& actors, [[maybe_unused]] std::shared_ptr<SceneCommonRegion> commonRegion)
        {
            update(actors, commonRegion);
            updateComponents();
        }

        void updateComponents()
        {
            for (auto& component : mComponentsVec)
                component->update();
        }

        //なければnullopt
        template<typename RequiredComponent>
        std::optional<std::shared_ptr<RequiredComponent>> getComponent() //なければ無効値、必ずチェックを(shared_ptrのoperator boolで判別可能)
        {
            const auto& iter = mComponents.find(typeid(RequiredComponent).hash_code());
            return (iter != mComponents.end()) ? std::make_optional(std::dynamic_pointer_cast<RequiredComponent>(iter->second)) : std::nullopt;
        }

    protected:

        template<typename Component>
        std::shared_ptr<Component> addComponent()
        {
            auto tmp = std::make_shared<Component>();
            mComponents.emplace(typeid(Component).hash_code(), tmp);
            mComponentsVec.emplace_back(tmp);
            return tmp;
        }

        //引数付きコンストラクタもOK
        template<typename Component, typename... Args>
        std::shared_ptr<Component> addComponent(Args... constructArgs)
        {
            auto tmp = std::make_shared<Component>(constructArgs...);
            mComponents.emplace(typeid(Component).hash_code(), tmp);
            mComponentsVec.emplace_back(tmp);
            return tmp;
        }

    private:
        //キーは型のハッシュ値
        std::unordered_map<size_t, std::shared_ptr<IComponent>> mComponents;
        std::vector<std::shared_ptr<IComponent>> mComponentsVec;
    };
};