#pragma once

#include <unordered_map>
#include <string>
#include <memory>

#include "IActor.hpp"

//Scene内のアクタを分離して各アクタに配布しやすいようにする
class ActorsInScene
{
public:

	template<typename Actor>
	void addActor(const std::string& actorName)
	{
		mActors.emplace(actorName, std::make_shared<Actor>());
	}

	template<typename Actor, typename... Args>
	void addActor(const std::string& actorName, Args... constructArgs)
	{
		mActors.emplace(actorName, std::make_shared<Actor>(constructArgs...));
	}

	template<typename RequiredActor>
	std::shared_ptr<RequiredActor> getActor(const std::string& actorName)//なければ無効値、必ずチェックを(shared_ptrのoperator boolで判別可能)
	{
		const auto& iter = mActors.find(actorName);
		return (iter != mActors.end()) ? std::dynamic_pointer_cast<RequiredActor>(iter->second) : nullptr;
	}

    void init()//アクタ類のinitはここで呼ばれる 忘れないようにStateのinitの最後に呼ぶように
	{
		for(auto& actor : mActors)
			actor.second->init(*this);
	}

	void update()
	{
		for(auto& actor : mActors)
			actor.second->update(*this);
	}

private:
    std::unordered_map<std::string, std::shared_ptr<IActor>> mActors;
};

