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
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <optional>
#include <exception>
#include <cassert>
#include <Cutlass.hpp>

#include "ActorsInScene.hpp"

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

		inline void initAll()
		{
			assert(mIsSetData);
			init();
		}

		void updateAll()
		{
			update();
			mActors.update();
		}

		void setInternalData
		(
			Application_t* application, 
			std::shared_ptr<CommonRegion> commonRegion,
			std::shared_ptr<Cutlass::Context> context,
			const std::vector<Cutlass::HWindow>& windows
		)
		{
			mApplication = application;
			mCommonRegion = commonRegion;
			mContext = context;
			mActors.setInternalData(commonRegion, context, windows);
			mIsSetData = true;
		}

	protected://子以外呼ばなくていいです

		void changeScene(const Key_t& dstSceneKey, bool cachePrevScene = false)
		{
			mApplication->changeScene(dstSceneKey, cachePrevScene);
		}

		void resetScene()
		{
			mActors.clearActors();
			initAll();
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

		std::shared_ptr<Cutlass::Context> const getContext() const
		{
			return mContext;
		}

		const std::vector<Cutlass::HWindow>& getHWindows() const
		{
			return mWindows;
		}

	private://メンバ変数
		bool mIsSetData;

		std::shared_ptr<CommonRegion> mCommonRegion;
		//std::unique_ptr<BaseRenderer> mRenderer;
		Application_t* mApplication;//コンストラクタにてnullptrで初期化
		ActorsInScene<CommonRegion> mActors;

		//Cutlass
		std::shared_ptr<Cutlass::Context> mContext;
		std::vector<Cutlass::HWindow> mWindows;
	};

	template<typename Key_t, typename CommonRegion>
	class Application
	{
	private://using宣言部

		using Scene_t = std::shared_ptr<Scene<Key_t, CommonRegion>>;
		using Factory_t = std::function<Scene_t()>;

	public://メソッド宣言部

		Application(const Cutlass::InitializeInfo& initializeInfo, const std::initializer_list<Cutlass::WindowInfo>& windowInfos)
		 : mCommonRegion(std::make_shared<CommonRegion>())
		 , mEndFlag(false)
		{
			mContext = std::make_shared<Cutlass::Context>();

			{//Context初期化
				if(Cutlass::Result::eSuccess != mContext->initialize(initializeInfo))
					std::cerr << "Failed to initialize cutlass context!\n";
			}

			mWindows.reserve(windowInfos.size());
			for(const auto& wi : windowInfos)
			{//window作成
				mWindows.emplace_back();
				if (Cutlass::Result::eSuccess != mContext->createWindow(wi, mWindows.back()))
					std::cerr << "Failed to create window!\n";
			}
		}

		Application(const Cutlass::InitializeInfo&& initializeInfo, const std::initializer_list<Cutlass::WindowInfo>&& windowInfos)
		 : mCommonRegion(std::make_shared<CommonRegion>())
		 , mEndFlag(false)
		{
			mContext = std::make_shared<Cutlass::Context>();

			{//Context初期化
				if(Cutlass::Result::eSuccess != mContext->initialize(initializeInfo))
					std::cerr << "Failed to initialize cutlass context!\n";
			}

			mWindows.reserve(windowInfos.size());
			for(const auto& wi : windowInfos)
			{//window作成
				mWindows.emplace_back();
				if (Cutlass::Result::eSuccess != mContext->createWindow(wi, mWindows.back()))
					std::cerr << "Failed to create window!\n";
			}
		}

		//Noncopyable, Nonmoveable
        Application(const Application&) = delete;
        Application &operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application &operator=(Application&&) = delete;

		~Application()
		{
			mContext->destroy();
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
		}

		void update()
		{
			//入力更新
			if (Cutlass::Result::eSuccess != mContext->updateInput())
				std::cerr << "Failed to update input!\n";
			//全体更新
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
					m->setInternalData(this, mCommonRegion, mContext, mWindows);
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
			return mEndFlag || mContext->shouldClose();
		}

	public://共有データはアクセスできるべき
		std::shared_ptr<CommonRegion> mCommonRegion;

	private:

		std::unordered_map<Key_t, Factory_t> mScenesFactory;
		std::pair<Key_t, Scene_t> mCurrent;
		std::optional<std::pair<Key_t, Scene_t>> mCache;
		std::optional<Key_t> mFirstSceneKey;//nulloptで初期化
		bool mEndFlag;

		//Cutlass
		std::shared_ptr<Cutlass::Context> mContext;
		std::vector<Cutlass::HWindow> mWindows;
	};
};

//#endif //!_Application_Scene_IMPL