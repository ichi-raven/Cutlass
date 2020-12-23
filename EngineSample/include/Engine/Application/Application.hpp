//前方宣言のみ
namespace Engine
{
template<typename Key_t, typename CommonRegion>
class Application;

template<typename Key_t, typename CommonRegion>
class Scene;
}

//init, update, render, endの宣言がめんどくさい人用
#define GEN_SCENE_HEADER_CLASS  \
public: \
virtual void init() override final;\
virtual void update() override final;\
private:

#define GEN_SCENE_HEADER_STRUCT \
virtual void init() override final;\
virtual void update() override final;\


//#ifndef _Application_Scene_IMPL//実装部は一度だけの展開
//#define _Application_Scene_IMPL
#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <optional>
#include <exception>
#include <cassert>

#include "../Components/MeshComponent.hpp"
#include "../Components/MaterialComponent.hpp"

#include "ActorsInScene.hpp"
#include "../System/BaseRenderer.hpp"

namespace Engine
{
	template<typename Key_t, typename CommonRegion>
	class Scene
	{
	private://using宣言部

		using Application_t = Application<Key_t, CommonRegion>;

	public://メソッド宣言部

		Scene()
		: mApplication(nullptr)
		, mIsSetData(false)
		{
			//コンストラクタです
		}

		// Scene(Application_t* application, const std::shared_ptr<CommonRegion>& commonRegion)
		// : mApplication(application)
		// , mCommonRegion(commonRegion)
		// , mActors(ActorsInScene(commonRegion))
		// {

		// }

		virtual void init() = 0;

		virtual void update() = 0;

		void initAll()
		{
			assert(mIsSetData);
			init();
			mActors.init();
		}

		void updateAll()
		{
			update();
			mActors.update();
		}

		void setInternalData(Application_t* application, const std::shared_ptr<CommonRegion>& commonRegion)
		{
			mApplication = application;
			mCommonRegion = commonRegion;
			mActors.setNewCommonRegion(commonRegion);
			mIsSetData = true;
		}

	protected://子以外呼ばなくていいです

		void changeScene(const Key_t& dstSceneKey, bool cachePrevScene = false)
		{
			mApplication->changeScene(dstSceneKey, cachePrevScene);
		}

		void exitApplication()
		{
			mApplication->dispatchEnd();
		}

		template<typename Actor>
		void addActor(const std::string& actorName)
		{
			mActors.template addActor<Actor>(actorName);
		}

		template<typename Actor, typename... Args>
		void addActor(const std::string& actorName, Args... constructArgs)
		{
			mActors.template addActor<Actor>(actorName, constructArgs...);
		}

		template<typename RequiredActor>
		std::optional<std::shared_ptr<RequiredActor>> getActor(const std::string& actorName)//なければ無効値、必ずチェックを(shared_ptrのoperator boolで判別可能)
		{
			return mActors.template getActor<RequiredActor>(actorName);
		}

		ActorsInScene<CommonRegion>& getActorsInScene()
		{
			return mActors;
		}

		std::shared_ptr<CommonRegion> const getCommonRegion() const
		{
			return mCommonRegion;
		}

	private://メンバ変数
		bool mIsSetData;

		std::shared_ptr<CommonRegion> mCommonRegion;
		//std::unique_ptr<BaseRenderer> mRenderer;
		Application_t* mApplication;//コンストラクタにてnullptrで初期化
		ActorsInScene<CommonRegion> mActors;
	};

	template<typename Key_t, typename CommonRegion>
	class Application
	{
	private://using宣言部

		using Scene_t = std::shared_ptr<Scene<Key_t, CommonRegion>>;
		using Factory_t = std::function<Scene_t()>;

	public://メソッド宣言部

		Application()
		{
			mFirstSceneKey = std::nullopt;
			mCommonRegion = std::make_shared<CommonRegion>();
			mEndFlag = false;
			mCache = std::nullopt;
		}

		void init(const Key_t& firstSceneKey)
		{
			mFirstSceneKey = firstSceneKey;
			mEndFlag = false;

			if (!mFirstSceneKey.has_value())
			{
	#ifdef _DEBUG
				assert(!"the first key does not exist!");
	#endif //_DEBUG
				//release時は止めない
				return;
			}

			mCurrent.first = mFirstSceneKey.value();
			mCurrent.second = mScenesFactory[mFirstSceneKey.value()]();
			mCurrent.second->initAll();
		}

		void update()
		{
			mCurrent.second->updateAll();
		}

		template<typename DerivedScene>
		void addScene(const Key_t& key)
		{
			if (mScenesFactory.find(key) != mScenesFactory.end())
			{
	#ifdef _DEBUG
				assert(!"this key already exist!");
	#endif //_DEBUG
				//release時は止めない
				return;
			}

			mScenesFactory.emplace
			(
				key,
				[&]()
				{
					//auto m = std::make_shared<DerivedScene>(this, mCommonRegion);//
					auto m = std::make_shared<DerivedScene>();
					m->setInternalData(this, mCommonRegion);
					m->initAll();

					return m;
				}
			);

			if (!mFirstSceneKey)//まだ値がなかったら
			{
				mFirstSceneKey = key;
			}
		}

		void changeScene(const Key_t& dstSceneKey, bool cachePrevScene = false)
		{
			if (mScenesFactory.find(dstSceneKey) == mScenesFactory.end())
			{
	#ifdef _DEBUG
				assert(!"this key does not exist!");
	#endif //_DEBUG
				//release時は止めない
				return;
			}

			if (cachePrevScene)
				mCache = mCurrent;
			

			if (mCache && dstSceneKey == mCache.value().first)
			{
				mCurrent = mCache.value();
				mCache = std::nullopt;
			}
			else
			{
				mCurrent.first = dstSceneKey;
				mCurrent.second = mScenesFactory[dstSceneKey]();
			}
		}

		void dispatchEnd()
		{
			mEndFlag = true;
		}

		bool endAll()
		{
			return mEndFlag;
		}

	public://共有データはアクセスできるべき
		std::shared_ptr<CommonRegion> mCommonRegion;

	private:

		std::unordered_map<Key_t, Factory_t> mScenesFactory;
		std::pair<Key_t, Scene_t> mCurrent;
		std::optional<std::pair<Key_t, Scene_t>> mCache;
		std::optional<Key_t> mFirstSceneKey;//nulloptで初期化
		bool mEndFlag;
	};
};

//#endif //!_Application_Scene_IMPL