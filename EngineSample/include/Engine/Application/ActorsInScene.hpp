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
			const std::shared_ptr<Cutlass::Context>& context,
			const std::shared_ptr<System> system
		)
		: mCommonRegion(commonRegion)
		, mContext(context)
		, mSystem(system)
		, mBeforeActorNum(0)
		{
			//チューニング対象?
			mActorsVec.reserve(5);
		}

		void lateinitActors()
		{
			for(size_t i = 0; i < mActorsVec.size(); ++i)
			{
				mActorsVec[i]->lateinit();
			}
		}

		template<typename Actor>
		std::shared_ptr<Actor> addActor(const std::string& actorName)
		{
			auto&& tmp = std::make_shared<Actor>(*this, mCommonRegion, mContext, mSystem);
			tmp->init();
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

		void forEachActors(const std::function<void(std::shared_ptr<IActor<CommonRegion>> actor)>& proc)
		{
			std::for_each(mActorsVec.begin(), mActorsVec.end(), proc);
		}

		//全てのアクタに対しての初期化処理, リセットしたいときとか
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
			uint32_t diff = mActorsVec.size() - mBeforeActorNum;
			if(diff > 0)
				for(auto& itr = mActorsVec.end() - diff - 1; itr != mActorsVec.end(); ++itr)
					itr->lateinit();
			

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