#include <Cutlass.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>

using namespace Cutlass;

//頂点型
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 UV;
};

//ユニフォームバッファ型
struct Uniform
{
    glm::mat4 world;
    glm::mat4 view;
    glm::mat4 proj;
};

int main()
{
    //定数
    constexpr uint32_t frameCount = 3;
    constexpr uint32_t width = 800, height = 600;
    
    //-----------------------------------------ジオメトリ定義
    const float k = 1.0f;
    const glm::vec3 red(1.0f, 0.0f, 0.0f);
    const glm::vec3 green(0.0f, 1.0f, 0.0f);
    const glm::vec3 blue(0.0f, 0.0f, 1.0f);
    const glm::vec3 white(1.0f);
    const glm::vec3 black(0.0f);
    const glm::vec3 yellow(1.0f, 1.0f, 0.0f);
    const glm::vec3 magenta(1.0f, 0.0f, 1.0f);
    const glm::vec3 cyan(0.0f, 1.0f, 1.0f);

    const glm::vec2 lb(0.0f, 0.0f);
    const glm::vec2 lt(0.0f, 1.0f);
    const glm::vec2 rb(1.0f, 0.0f);
    const glm::vec2 rt(1.0f, 1.0f);

    const glm::vec3 nf(0, 0, 1.f);
    const glm::vec3 nb(0, 0, -1.f);
    const glm::vec3 nr(1.f, 0, 0);
    const glm::vec3 nl(-1.f, 0, 0);
    const glm::vec3 nu(0, 1.f, 0);
    const glm::vec3 nd(0, -1.f, 0);

    std::vector<Vertex> vertices =
    {
        // 正面
        {glm::vec3(-k, k, k), yellow, nf, lb},
        {glm::vec3(-k, -k, k), red, nf, lt},
        {glm::vec3(k, k, k), white, nf, rb},
        {glm::vec3(k, -k, k), magenta, nf, rt},
        // 右
        {glm::vec3(k, k, k), white, nr, lb},
        {glm::vec3(k, -k, k), magenta, nr, lt},
        {glm::vec3(k, k, -k), cyan, nr, rb},
        {glm::vec3(k, -k, -k), blue, nr, rt},
        // 左
        {glm::vec3(-k, k, -k), green, nl, lb},
        {glm::vec3(-k, -k, -k), black, nl, lt},
        {glm::vec3(-k, k, k), yellow, nl, rb},
        {glm::vec3(-k, -k, k), red, nl, rt},
        // 裏
        {glm::vec3(k, k, -k), cyan, nb, lb},
        {glm::vec3(k, -k, -k), blue, nb, lt},
        {glm::vec3(-k, k, -k), green, nb, rb},
        {glm::vec3(-k, -k, -k), black, nb, rt},
        // 上
        {glm::vec3(-k, k, -k), green, nu, lb},
        {glm::vec3(-k, k, k), yellow, nu, lt},
        {glm::vec3(k, k, -k), cyan, nu, rb},
        {glm::vec3(k, k, k), white, nu, rt},
        // 底
        {glm::vec3(-k, -k, k), red, nd, lb},
        {glm::vec3(-k, -k, -k), black, nd, lt},
        {glm::vec3(k, -k, k), magenta, nd, rb},
        {glm::vec3(k, -k, -k), blue, nd, rt},
    };

    std::vector<uint32_t> indices =
    {
        0, 2, 1, 1, 2, 3,    // front
        4, 6, 5, 5, 6, 7,    // right
        8, 10, 9, 9, 10, 11, // left

        12, 14, 13, 13, 14, 15, // back
        16, 18, 17, 17, 18, 19, // top
        20, 22, 21, 21, 22, 23, // bottom
    };

    //-----------------------------------------------------

    //コンテキスト取得
    Context& context = Context::getInstance();

    {
        InitializeInfo ii("test", true);
        if (Result::eSuccess != context.initialize(ii))
            std::cout << "Failed to Initialize!\n";
    }

   HWindow window;
    {//ウィンドウ作成
        WindowInfo wi(width, height, frameCount, "CutlassTest", false, true);
        if (Result::eSuccess != context.createWindow(wi, window))
            std::cout << "Failed to create window!";
    }

    HBuffer vertexBuffer;
    {//頂点バッファ作成, 書き込み
        BufferInfo bi;
        bi.setVertexBuffer<decltype(vertices[0])>(vertices.size());
        if (Result::eSuccess != context.createBuffer(bi, vertexBuffer))
            std::cout << "Failed to create vertex buffer!\n";
        if (Result::eSuccess != context.writeBuffer(bi.size, vertices.data(), vertexBuffer))
            std::cout << "Failed to write vertex buffer!\n";
    }

    HBuffer indexBuffer;
    {//インデックスバッファ作成, 書き込み
        BufferInfo bi;
        bi.setIndexBuffer<decltype(indices[0])>(indices.size());
        if (Result::eSuccess != context.createBuffer(bi, indexBuffer))
            std::cout << "Failed to create index buffer!\n";
        if (Result::eSuccess != context.writeBuffer(bi.size, indices.data(), indexBuffer))
            std::cout << "Failed to write index buffer!\n";
    }

    HBuffer renderUB;
    std::vector<HBuffer> presentUBs(frameCount);
    {//ユニフォームバッファ作成
        BufferInfo bi;
        bi.setUniformBuffer<Uniform>();
        
        if (Result::eSuccess != context.createBuffer(bi, renderUB))
            std::cout << "Failed to create uniform\n";
        
        for (auto& ub : presentUBs)
            if (Result::eSuccess != context.createBuffer(bi, ub))
                std::cout << "Failed to create uniform\n";
    }

    HTexture texture;
    {//テクスチャ作成
        if (Result::eSuccess != context.createTextureFromFile("../Textures/texture.png", texture))
            std::cout << "Failed to create texture from file!\n";
    }

    HTexture target;
    {//描画先用テクスチャ作成
        TextureInfo ti;
        ti.setRTTex2D(width, height, ResourceType::eUNormVec4);
        if (Result::eSuccess != context.createTexture(ti, target))
            std::cout << "Failed to create render target texture!\n";
    }

    HRenderDST texDST;
    {//テクスチャ用描画先オブジェクト作成
        if (Result::eSuccess != context.createRenderDST({ target }, texDST))
            std::cout << "Failed to create texture renderDST\n";
    }

    HRenderDST renderDST;
    {//ウィンドウ用描画先オブジェクト作成
        if (Result::eSuccess != context.createRenderDST(window, true, renderDST))
            std::cout << "Failed to create renderDST\n";
    }

    HRenderPipeline renderPipeline, presentPipeline;
    {//テクスチャ描画用パス、ウィンドウ描画用パスを定義

        //頂点レイアウト定義
        VertexLayout vl;
        vl.set(ResourceType::eF32Vec3, "pos");
        vl.set(ResourceType::eF32Vec3, "color");
        vl.set(ResourceType::eF32Vec3, "normal");
        vl.set(ResourceType::eF32Vec2, "UV");

        //シェーダリソースレイアウト定義
        ShaderResourceDesc SRDesc;
        SRDesc.layout.allocForUniformBuffer(0);
        SRDesc.layout.allocForCombinedTexture(1);
        SRDesc.setCount = frameCount;

        RenderPipelineInfo rpi
        (
            vl,
            ColorBlend::eDefault,
            Topology::eTriangleList,
            RasterizerState(),
            MultiSampleState::eDefault,
            DepthStencilState::eNone,
            Shader("../Shaders/vert.spv", "main"),
            Shader("../Shaders/frag.spv", "main"),
            SRDesc,
            texDST
        );

        if (Result::eSuccess != context.createRenderPipeline(rpi, renderPipeline))
            std::cout << "Failed to create render pipeline!\n";

        //2パス目は対象とデプスバッファリングが変わる
        rpi.depthStencilState = DepthStencilState::eDepth;
        rpi.renderDST = renderDST;

        if (Result::eSuccess != context.createRenderPipeline(rpi, presentPipeline))
            std::cout << "Failed to create render pipeline2!\n";
    }

    ShaderResourceSet renderSet;
    {//テクスチャレンダリングパスのリソースセット

        renderSet.setUniformBuffer(0, renderUB);
        renderSet.setCombinedTexture(1, texture);
        
    }

    std::vector<ShaderResourceSet> presentSets(frameCount);
    {//ウィンドウに描画するパスのリソースセット
        for (size_t i = 0; i < presentSets.size(); ++i)
        {
            presentSets[i].setUniformBuffer(0, presentUBs[i]);
            presentSets[i].setCombinedTexture(1, target);
        }
    }

    CommandList renderCL;
    std::vector<CommandList> presentCL(frameCount);

    {//コマンドリストを作成
        ColorClearValue ccv{ 0, 0.5f, 0, 0 };
        DepthClearValue dcv(1.f, 0);

        renderCL.bindVB(vertexBuffer);
        renderCL.bindIB(indexBuffer);
        
        renderCL.beginRenderPipeline(renderPipeline, ccv, dcv);
        renderCL.bindSRSet(renderSet);
        renderCL.renderIndexed(indices.size(), 1, 0, 0, 0);
        renderCL.endRenderPipeline();
        renderCL.sync();
        

        for(size_t i = 0; i < presentCL.size(); ++i)
        {
            presentCL[i].bindVB(vertexBuffer);
            presentCL[i].bindIB(indexBuffer);

            presentCL[i].beginRenderPipeline(presentPipeline);
            presentCL[i].bindSRSet(presentSets[i]);
            presentCL[i].renderIndexed(indices.size(), 1, 0, 0, 0);
            presentCL[i].endRenderPipeline();
            presentCL[i].present();
        }  
    }

    HCommandBuffer renderCB, presentCB;
    {//リストからGPUでバッファを構築
        if (Result::eSuccess != context.createCommandBuffer(renderCL, renderCB))
            std::cout << "Failed to create command buffer\n";
        if (Result::eSuccess != context.createCommandBuffer(presentCL, presentCB))
            std::cout << "Failed to create command buffer\n";
    }

    {//メインループ
        int frame = 0;

        //10F平均でFPSを計測
        std::array<double, 10> times;
        std::chrono::high_resolution_clock::time_point now, prev = std::chrono::high_resolution_clock::now();
        //カメラの移動スピード、座標
        constexpr double speed = 0.5f;
        glm::vec3 pos(0, 0, 10.f);

        //ウィンドウ破棄の通知もしくはEscキーで終了
        while (!context.shouldClose() && !context.getKey(Key::Escape))
        {
            //入出力更新
            if (Result::eSuccess != context.updateInput())
                std::cerr << "Failed to handle event!\n";

            {//各種情報表示
                now = std::chrono::high_resolution_clock::now();
                times[frame % 10] = std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count() / 1000000.;
                std::cout << "now frame : " << frame << "\n";
                std::cout << "fps : " << 1. / (std::accumulate(times.begin(), times.end(), 0.) / 10.) << "\n";
                double x, y;
                context.getMouse(x, y);
                std::cout << "mouse x: " << x << " y: " << y << "\n";
            }

            {//カメラを移動してみる
                if (context.getKey(Key::W))
                    pos.z -= speed;
                if (context.getKey(Key::S))
                    pos.z += speed;
                if (context.getKey(Key::A))
                    pos.x -= speed;
                if (context.getKey(Key::D))
                    pos.x += speed;
                if (context.getKey(Key::Up))
                    pos.y += speed;
                if (context.getKey(Key::Down))
                    pos.y -= speed;
            }

            {//UBO書き込み
                Uniform ubo;
                ubo.world = glm::rotate(glm::identity<glm::mat4>(), glm::radians(3.f * frame), glm::vec3(0, 1.f, 0));
                ubo.view = glm::lookAtRH(pos, pos + glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
                ubo.proj = glm::perspective(glm::radians(45.f), 1.f * width / height, 1.f, 100.f);
                ubo.proj[1][1] *= -1;

                uint32_t frameIndex = context.getFrameBufferIndex(renderDST);
                if (Result::eSuccess != context.writeBuffer(sizeof(Uniform), &ubo, renderUB))
                    std::cout << "Failed to write uniform buffer!\n";
                if (Result::eSuccess != context.writeBuffer(sizeof(Uniform), &ubo, presentUBs[(frameIndex + 1) % frameCount]))
                    std::cout << "Failed to write uniform buffer!\n";
            }

            //コマンド実行
            if (Result::eSuccess != context.execute(renderCB))
                std::cerr << "Failed to execute command!\n";
            if (Result::eSuccess != context.execute(presentCB))
               std::cerr << "Failed to execute command!\n";

            {//更新
                ++frame;
                prev = now;
            }
        }
    }

    //破棄処理、明示的に行っているがユーザが行わなくてもよい
    context.destroy();

    return 0;
}