#pragma once

#include <typeinfo>
#include <memory>
#include <unordered_map>
#include <Cutlass/Cutlass.hpp>

#include "SceneCommonRegion.hpp"
#include "IComponent.hpp"

class ActorsInScene;

//アクタのクラスのヘッダにこれを書けばヘッダが自動生成できます
#define GEN_ACTOR_CLASS(CLASSNAME) \
public:\
virtual ~CLASSNAME() final override;\
virtual void init([[maybe_unused]] ActorsInScene& actors) final override;\
virtual void update([[maybe_unused]] ActorsInScene& actors) final override;\
private:

//init, updateの引数自動生成(ARG_NAME...引数名)
#define INIT_ARG_ACTORS(ARG_NAME) [[maybe_unused]] ActorsInScene& ARG_NAME
#define UPDATE_ARG_ACTORS(ARG_NAME) [[maybe_unused]] ActorsInScene& ARG_NAME

//[[maybe_unused]]とは、その引数を関数内で使用しなかったとしてもコンパイラに警告を吐かせない属性です
//よくわからない場合は上記マクロを使用すればいいかも?

class IActor
{
public:
    IActor()
    {}

    virtual ~IActor() = 0;

    virtual void init([[maybe_unused]] ActorsInScene& actors) = 0;//注意 : 他アクタは取得できますが、望む情報が得られない可能性があります

    virtual void update([[maybe_unused]] ActorsInScene& actors) = 0;

    void updateComponents()
    {
        for (auto& component : mComponents)
            component.second->update();
    }

    template<typename RequiredComponent>
    std::shared_ptr<RequiredComponent> getComponent() //なければ無効値、必ずチェックを(shared_ptrのoperator boolで判別可能)
    {
        const auto& iter = mComponents.find(typeid(RequiredComponent).hash_code());
        return (iter != mComponents.end()) ? std::dynamic_pointer_cast<RequiredComponent>(iter->second) : nullptr;
    }

protected:

    template<typename Component>
    void addComponent()
    {
        mComponents.emplace(typeid(Component).hash_code(), std::make_shared<Component>());
    }

    //引数付きコンストラクタもOK
    template<typename Component, typename... Args>
    void addComponent(Args... constructArgs)
    {
        mComponents.emplace(typeid(Component).hash_code(), std::make_shared<Component>(constructArgs...));
    }

private:
    //キーは型のハッシュ値
    std::unordered_map<size_t, std::shared_ptr<IComponent>> mComponents;
};