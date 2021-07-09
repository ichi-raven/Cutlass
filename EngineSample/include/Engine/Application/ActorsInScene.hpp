namespace Engine
{
	template<typename CommonRegion>
	class ActorsinScene;
};

#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <algorithm>
#include <iostream>

#include "../Actors/IActor.hpp"

//Scene内のアクタを分離して各アクタに配布しやすいようにする
namespace Engine
{
	class System;

	template<typename CommonRegion>
	class ActorsInScene
	{
	public:

		ActorsInScene() = delete;

		ActorsInScene
		(
			std::shared_ptr<CommonRegion> const commonRegion,
			const std::shared_ptr<Cutlass::Context> context,
			const std::shared_ptr<System> system
		)
		: mCommonRegion(commonRegion)
		, mContext(context)
		, mSystem(system)
		, mBeforeActorNum(0)
		{
			//チューニング対象?
			mActorsVec.reserve(8);
		}

		//autoInitをオンにするとinitを自分で呼ぶ必要がなくなりますが、循環参照を起こす可能性があります
		template<typename Actor>
		std::shared_ptr<Actor> addActor(const std::string_view actorName, bool autoInit = false)
		{
			auto tmp = std::make_shared<Actor>(*this, mCommonRegion, mContext, mSystem);
			tmp->awake();
			mActors.emplace(static_cast<std::string>(actorName), tmp);
			mActorsVec.emplace_back(tmp);
			if(autoInit)
				tmp->init();
			return tmp;
		}

		void removeActor(const std::string_view actorName)
		{
			auto&& itr = mActors.find(static_cast<std::string>(actorName));
			if(itr == mActors.end())
				return;
			mRemovedActors.push(itr->second);
			mActors.erase(itr);
		}

		template<typename RequiredActor>
		std::optional<std::shared_ptr<RequiredActor>> getActor(const std::string_view actorName)//なければ無効値、必ずチェックを(shared_ptrのoperator boolで判別可能)
		{
			const auto iter = mActors.find(static_cast<std::string>(actorName));
			return (iter != mActors.end()) ? std::make_optional(std::dynamic_pointer_cast<RequiredActor>(iter->second)) : std::nullopt;
		}

		void forEachActors(const std::function<void(const std::shared_ptr<IActor<CommonRegion>>& actor)>& proc)
		{
			std::for_each(mActorsVec.begin(), mActorsVec.end(), proc);
		}

		//緊急SOS! SceneのActor全部消す
		void clearActors()
		{
			mActors.clear();
			mActorsVec.clear();
			//queueをclear
			std::swap(std::queue<std::shared_ptr<IActor<CommonRegion>>>(), mRemovedActors);
		}

		//全てのアクタに対しての更新処理、ユーザは呼ぶ必要はありません
		void update()
		{
			//ついでに削除しちゃう
			auto&& end = mActorsVec.end();
			auto&& itr = std::remove_if(mActorsVec.begin(), end, [&](std::shared_ptr<IActor<CommonRegion>>& actor)
			{
				if(!mRemovedActors.empty() && actor == mRemovedActors.back())
				{
					mRemovedActors.pop();
					return true;
				}

				actor->updateAll();
				return false;
			});

			//削除
			mActorsVec.erase(itr, end);
			mBeforeActorNum = mActorsVec.size();
		}

	private:
		std::unordered_map<std::string, std::shared_ptr<IActor<CommonRegion>>> mActors;
		std::vector<std::shared_ptr<IActor<CommonRegion>>> mActorsVec;
		std::queue<std::shared_ptr<IActor<CommonRegion>>> mRemovedActors;
		
		std::shared_ptr<CommonRegion> mCommonRegion;
		
		std::shared_ptr<Cutlass::Context> mContext;
		std::shared_ptr<System> mSystem;
		uint32_t mBeforeActorNum;
	};
};