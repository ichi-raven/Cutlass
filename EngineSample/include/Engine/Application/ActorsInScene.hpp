#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <algorithm>

#include "../Actors/IActor.hpp"

//Scene内のアクタを分離して各アクタに配布しやすいようにする
namespace Engine
{
	template<typename SceneCommonRegion>
	class ActorsInScene
	{
	public:

		ActorsInScene()
		{
			//チューニング対象?
			mActorsVec.reserve(7);
		}

		//呼ばずに実行すると壊れるぞ!(そもそもここ関連の部分を弄らないでください)
		void setNewCommonRegion(const std::shared_ptr<SceneCommonRegion>& sceneCommonRegion)
		{
			mCommonRegion = sceneCommonRegion;
		}

		template<typename Actor>
		std::shared_ptr<Actor> addActor(const std::string& actorName)
		{
			auto tmp = std::make_shared<Actor>();
			tmp->template init(*this, mCommonRegion);
			mActors.emplace(actorName, tmp);
			mActorsVec.emplace_back(tmp);
			return tmp;
		}

		template<typename Actor, typename... Args>
		std::shared_ptr<Actor> addActor(const std::string& actorName, Args... constructArgs)
		{
			auto tmp = std::make_shared<Actor>(constructArgs...);
			tmp->template init(*this, mCommonRegion);
			mActors.emplace(actorName, tmp);
			mActorsVec.emplace_back(tmp);
			return tmp;
		}

		void removeActor(const std::string& actorName)
		{
			auto&& itr = mActors.find(actorName);
			if(itr == mActors.end())
				return;
			mRemovedActors.push(itr->second);
			mActors.erase(itr);
		}

		template<typename RequiredActor>
		std::optional<std::shared_ptr<RequiredActor>> getActor(const std::string& actorName)//なければ無効値、必ずチェックを(shared_ptrのoperator boolで判別可能)
		{
			const auto& iter = mActors.find(actorName);
			return (iter != mActors.end()) ? std::make_optional(std::dynamic_pointer_cast<RequiredActor>(iter->second)) : std::nullopt;
		}

		void forEachActor(const std::function<void(std::shared_ptr<IActor<SceneCommonRegion>> actor)>& proc)
		{
			std::for_each(mActorsVec.begin(), mActorsVec.end(), proc);
		}

		//全てのアクタに対しての初期化処理, リセットしたいときとか
		void init()
		{
			for(auto& actor : mActorsVec)
				actor->init(*this, mCommonRegion);
		}

		//全てのアクタに対しての更新処理、Scene::updateActorsを呼べばユーザは呼ぶ必要はありません
		void update()
		{
			//ついでに削除しちゃう
			auto&& end = mActorsVec.end();
			auto&& itr = std::remove_if(mActorsVec.begin(), end, [&](std::shared_ptr<IActor<SceneCommonRegion>>& actor)
			{
				if(!mRemovedActors.empty() && actor == mRemovedActors.back())
				{
					mRemovedActors.pop();
					return true;
				}

				actor->updateAll(*this, mCommonRegion);
				return false;
			});

			//削除
			mActorsVec.erase(itr, end);
		}

	private:
		std::unordered_map<std::string, std::shared_ptr<IActor<SceneCommonRegion>>> mActors;
		std::vector<std::shared_ptr<IActor<SceneCommonRegion>>> mActorsVec;
		std::queue<std::shared_ptr<IActor<SceneCommonRegion>>> mRemovedActors;
		std::shared_ptr<SceneCommonRegion> mCommonRegion;
	};
};