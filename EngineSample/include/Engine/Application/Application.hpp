//前方宣言のみ
namespace Engine
{
	template<typename Key_t, typename CommonRegion>
	class Application;

	template<typename Key_t, typename CommonRegion>
	class Scene;
}

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
#include "../System/System.hpp"


//自作Sceneの定義にはこれを使ってください
//ヘッダに書けばオーバーロードすべき関数の定義はすべて完了します
#define GEN_SCENE(SCENE_TYPE, KEY_TYPE, COMMONREGION_TYPE)  \
public: \
SCENE_TYPE(Engine::Application<KEY_TYPE, COMMONREGION_TYPE>* application, std::shared_ptr<COMMONREGION_TYPE> commonRegion,std::shared_ptr<Cutlass::Context> context, std::shared_ptr<Engine::System> system) : Scene(application, commonRegion, context, system){}\
virtual ~SCENE_TYPE() override;\
virtual void init() override;\
virtual void update() override;\
private:

namespace Engine
{
	template<typename Key_t, typename CommonRegion>
	class Scene
	{
	private://using宣言部

		using Application_t = Application<Key_t, CommonRegion>;

	public://メソッド宣言部

		Scene() = delete;

		Scene
		(
			Application_t* application, 
			std::shared_ptr<CommonRegion> commonRegion,
			std::shared_ptr<Cutlass::Context> context,
			std::shared_ptr<System> system
		)
		: mApplication(application)
		, mCommonRegion(commonRegion)
		, mContext(context)
		, mSystem(system)
		, mActors(commonRegion, context, system)
		{

		}

		virtual ~Scene(){};

		virtual void init() = 0;

		virtual void update() = 0;

		inline void initAll()
		{
			init();
		}

		void updateAll()
		{
			update();
			mActors.update();
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

		std::shared_ptr<System> const getSystem() const
		{
			return mSystem;
		}

	private://メンバ変数

		std::shared_ptr<CommonRegion> mCommonRegion;
		Application_t* mApplication;//コンストラクタにてnullptrで初期化
		ActorsInScene<CommonRegion> mActors;

		std::shared_ptr<System> mSystem;
		
		//Cutlass
		std::shared_ptr<Cutlass::Context> mContext;
		std::vector<Cutlass::HWindow> mHWindows;
	};

	template<typename Key_t, typename CommonRegion>
	class Application
	{
	private://using宣言部

		using Scene_t = std::shared_ptr<Scene<Key_t, CommonRegion>>;
		using Factory_t = std::function<Scene_t()>;

	public://メソッド宣言部

		template<typename T>
		Application(const Cutlass::InitializeInfo& initializeInfo, const std::initializer_list<Cutlass::WindowInfo>& windowInfos)
		 : mCommonRegion(std::make_shared<CommonRegion>())
		 , mEndFlag(false)
		{
			mContext = std::make_shared<Cutlass::Context>();

			{//Context初期化
				if(Cutlass::Result::eSuccess != mContext->initialize(initializeInfo))
					std::cerr << "Failed to initialize cutlass context!\n";
			}

			mHWindows.reserve(windowInfos.size());
			for(const auto& wi : windowInfos)
				if (Cutlass::Result::eSuccess != mContext->createWindow(wi, mHWindows.emplace_back()))
					std::cerr << "Failed to create window!\n";

			initSystem();
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

			mHWindows.reserve(windowInfos.size());
			for(const auto& wi : windowInfos)
				if (Cutlass::Result::eSuccess != mContext->createWindow(wi, mHWindows.emplace_back()))
					std::cerr << "Failed to create window!\n";

			initSystem();
		}

		//Systemを差し替える場合に使ってください
		//System内の各親を継承していない場合多分壊れます
		template<typename InheritedRenderer = Renderer, typename InheritedLoader = Loader>
		void initSystem()
		{
			//ApplicationごとにSystem内部を選べれば色々できると思う
			mSystem = std::make_shared<System>();
			mSystem->renderer = std::make_unique<InheritedRenderer>(mContext, mHWindows);
			mSystem->loader = std::make_unique<InheritedLoader>();
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

			//開始シーンが設定されてない
			assert(mFirstSceneKey);

			mCurrent.first = mFirstSceneKey.value();
			mCurrent.second = mScenesFactory[mFirstSceneKey.value()]();
		}

		void update()
		{
			#ifdef _DEBUG
			//入力更新
			if (Cutlass::Result::eSuccess != mContext->updateInput())
				std::cerr << "Failed to update input!\n";
			#else
				 mContext->updateInput();
			#endif

			//全体更新
			mCurrent.second->updateAll();
		}

		template<typename InheritedScene>
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
					auto&& m = std::make_shared<InheritedScene>(this, mCommonRegion, mContext, mSystem);
					m->initAll();

					return m;
				}
			);

			if (!mFirstSceneKey)//まだ値がなかったら
				mFirstSceneKey = key;
			
		}

		void changeScene(const Key_t& dstSceneKey, bool cachePrevScene = false)
		{
			//そのシーンはない
			assert(mScenesFactory.find(dstSceneKey) != mScenesFactory.end());

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

	public:

		std::shared_ptr<CommonRegion> mCommonRegion;
		std::shared_ptr<System> mSystem;
		//Cutlass
		std::shared_ptr<Cutlass::Context> mContext;
		std::vector<Cutlass::HWindow> mHWindows;

	private:

		std::unordered_map<Key_t, Factory_t> mScenesFactory;
		std::pair<Key_t, Scene_t> mCurrent;
		std::optional<std::pair<Key_t, Scene_t>> mCache;
		std::optional<Key_t> mFirstSceneKey;//nulloptで初期化
		bool mEndFlag;
		
	};
};

//#endif //!_Application_Scene_IMPL