#include <Cutlass.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <cassert>

using namespace Cutlass;

//���_�^
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 UV;
};

//���j�t�H�[���o�b�t�@�^
struct Uniform
{
    glm::mat4 world;
    glm::mat4 view;
    glm::mat4 proj;
};

//���_�J���[�ŗ֊s����`�悵�A���̂��ƃe�N�X�`����\���ăV�F�[�f�B���O�t���Ńe�N�X�`�������_�����O���ĕ\�����Ă��܂�

int main()
{
    //�萔
    constexpr uint32_t frameCount = 3;
    constexpr uint32_t width = 1000, height = 800;
    
    //-----------------------------------------�W�I���g����`(������)
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
        // ����
        {glm::vec3(-k, k, k), yellow, nf, lb},
        {glm::vec3(-k, -k, k), red, nf, lt},
        {glm::vec3(k, k, k), white, nf, rb},
        {glm::vec3(k, -k, k), magenta, nf, rt},
        // �E
        {glm::vec3(k, k, k), white, nr, lb},
        {glm::vec3(k, -k, k), magenta, nr, lt},
        {glm::vec3(k, k, -k), cyan, nr, rb},
        {glm::vec3(k, -k, -k), blue, nr, rt},
        // ��
        {glm::vec3(-k, k, -k), green, nl, lb},
        {glm::vec3(-k, -k, -k), black, nl, lt},
        {glm::vec3(-k, k, k), yellow, nl, rb},
        {glm::vec3(-k, -k, k), red, nl, rt},
        // ��
        {glm::vec3(k, k, -k), cyan, nb, lb},
        {glm::vec3(k, -k, -k), blue, nb, lt},
        {glm::vec3(-k, k, -k), green, nb, rb},
        {glm::vec3(-k, -k, -k), black, nb, rt},
        // ��
        {glm::vec3(-k, k, -k), green, nu, lb},
        {glm::vec3(-k, k, k), yellow, nu, lt},
        {glm::vec3(k, k, -k), cyan, nu, rb},
        {glm::vec3(k, k, k), white, nu, rt},
        // ��
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

    //�R���e�L�X�g�쐬
    Context context;
    if (Result::eSuccess != context.initialize("SimpleSampleApp", true))
        assert(!"Failed to initialize!");

    HWindow window;
    {//�E�B���h�E�쐬
        WindowInfo wi(width, height, frameCount, "SimpleSampleWindow", false, true);
        if (Result::eSuccess != context.createWindow(wi, window))
            assert(!"Failed to create window!");
    }

    HBuffer vertexBuffer;
    {//���_�o�b�t�@�쐬, ��������
        BufferInfo bi;
        bi.setVertexBuffer<decltype(vertices[0])>(vertices.size());
        if (Result::eSuccess != context.createBuffer(bi, vertexBuffer))
            assert(!"Failed to create vertex buffer!");
        if (Result::eSuccess != context.writeBuffer(bi.size, vertices.data(), vertexBuffer))
            assert(!"Failed to write to vertex buffer!");
    }

    HBuffer indexBuffer;
    {//�C���f�b�N�X�o�b�t�@�쐬, ��������
        BufferInfo bi;
        bi.setIndexBuffer<decltype(indices[0])>(indices.size());
        if (Result::eSuccess != context.createBuffer(bi, indexBuffer))
            assert(!"Failed to create index buffer!");
        if (Result::eSuccess != context.writeBuffer(bi.size, indices.data(), indexBuffer))
            assert(!"Failed to write to index buffer!");
    }

    HBuffer renderUB;
    {//���j�t�H�[���o�b�t�@�쐬
        BufferInfo bi;
        bi.setUniformBuffer<Uniform>();
        if (Result::eSuccess != context.createBuffer(bi, renderUB))
            assert(!"Failed to create uniform");
    }

    HTexture texture;
    {//�e�N�X�`���쐬
        if (Result::eSuccess != context.createTextureFromFile("../Textures/texture.png", texture))
            assert(!"Failed to create texture from file!");
    }

    HTexture target;
    {//�`���p�e�N�X�`���쐬
        TextureInfo ti(width, height);
        ti.setRTTex2DColor(width, height);
        if (Result::eSuccess != context.createTexture(ti, target))
            assert(!"Failed to create render target texture!");
    }

    HTexture depthTarget;
    {//�f�v�X�o�b�t�@�p�e�N�X�`���쐬
        TextureInfo ti;
        ti.setRTTex2DDepth(width, height);
        if (Result::eSuccess != context.createTexture(ti, depthTarget))
            assert(!"Failed to create render target texture!");
    }

    HRenderPass contourPass, texPass, presentPass;
    if(Result::eSuccess != context.createRenderPass(RenderPassInfo(target, depthTarget), contourPass))
        assert(!"Failed to create contour render pass!");
    if(Result::eSuccess != context.createRenderPass(RenderPassInfo(target, depthTarget, true), texPass))
        assert(!"Failed to create contour render pass!");
    if(Result::eSuccess != context.createRenderPass(RenderPassInfo(window), presentPass))
        assert(!"Failed to create contour render pass!");

    HGraphicsPipeline contourPipeline, renderPipeline, presentPipeline;
    {//�e�N�X�`���`��p�p�X�A�E�B���h�E�`��p�p�X���`

        Shader VS("../Shaders/TexturedCube/vert.spv", "main");
        Shader FS("../Shaders/TexturedCube/frag.spv", "main"); 

        {//���o�͕ϐ����`�F�b�N

            auto out = VS.getOutputVariables();
            auto in = FS.getInputVariables();
            assert(out.size() == in.size());
            for(size_t i = 0; i < out.size(); ++i)
            {
                if(out[i].first != in[i].first)
                    assert(!"Layout Error!");
            }
        }

        GraphicsPipelineInfo gpi
        (
            Shader("../Shaders/TexturedCube/vert2.spv", "main"),
            Shader("../Shaders/TexturedCube/frag2.spv", "main"),
            contourPass,
            DepthStencilState::eDepth,  
            RasterizerState(PolygonMode::eFill, CullMode::eBack, FrontFace::eClockwise)
        );

        GraphicsPipelineInfo gpi2
        (
            VS,
            FS,
            texPass,
            DepthStencilState::eDepth,
            RasterizerState(PolygonMode::eFill, CullMode::eBack, FrontFace::eClockwise)
        );

        if (Result::eSuccess != context.createGraphicsPipeline(gpi, contourPipeline))
            assert(!"Failed to create render pipeline!");

        if (Result::eSuccess != context.createGraphicsPipeline(gpi2, renderPipeline))
            assert(!"Failed to create render pipeline!"); 
    }

    {
        GraphicsPipelineInfo gpi
        (
            Shader("../Shaders/present/vert.spv", "main"),
            Shader("../Shaders/present/frag.spv", "main"),
            presentPass,
            DepthStencilState::eNone,
            RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise),
            Topology::eTriangleStrip
        );

        if (Result::eSuccess != context.createGraphicsPipeline(gpi, presentPipeline))
            assert(!"Failed to create present pipeline!");  
    }

    ShaderResourceSet contourSet, renderSet;
    {//�e�N�X�`�������_�����O�p�X�̃��\�[�X�Z�b�g
        contourSet.bind(0, renderUB);

        renderSet.bind(0, renderUB);
        renderSet.bind(1, texture);
    }

    std::vector<ShaderResourceSet> presentSets(frameCount);
    {//�E�B���h�E�ɕ`�悷��p�X�̃��\�[�X�Z�b�g
        for (size_t i = 0; i < presentSets.size(); ++i)
            presentSets[i].bind(0, target);
    }

    CommandList contourCL, renderCL;
    std::vector<CommandList> presentCL(frameCount);

    {//�R�}���h���X�g���쐬
        ColorClearValue ccv{ 0, 0, 1.f, 1.f };
        ColorClearValue ccv2{0.3, 0.3, 0.3, 1.f };
        DepthClearValue dcv(1.f, 0);

        {
            contourCL.begin(contourPass, true, ccv, dcv);
            contourCL.bind(contourPipeline);
            contourCL.bind(vertexBuffer, indexBuffer);
            contourCL.bind(0, contourSet);
            contourCL.renderIndexed(indices.size(), 1, 0, 0, 0);
            contourCL.end();
        }
        
        {
            renderCL.begin(texPass, true);
            renderCL.bind(renderPipeline);
            renderCL.bind(vertexBuffer, indexBuffer);
            renderCL.bind(0, renderSet);
            renderCL.renderIndexed(indices.size(), 1, 0, 0, 0);
            renderCL.end();
        }

        for(size_t i = 0; i < presentCL.size(); ++i)
        {
            presentCL[i].barrier(target);
            presentCL[i].begin(presentPass, true, ccv, dcv);
            presentCL[i].bind(presentPipeline);
            presentCL[i].bind(0, presentSets[i]);
            presentCL[i].render(4, 1, 0, 0);
            presentCL[i].end();
        }
    }

    HCommandBuffer contourCB, renderCB, presentCB;
    {//���X�g����GPU�Ńo�b�t�@���\�z
        if (Result::eSuccess != context.createCommandBuffer(contourCL, contourCB))
            assert(!"Failed to create command buffer");
        if (Result::eSuccess != context.createCommandBuffer(renderCL, renderCB))
            assert(!"Failed to create command buffer");
        if (Result::eSuccess != context.createCommandBuffer(presentCL, presentCB))
            assert(!"Failed to create command buffer");
    }
    {//���C�����[�v
        int frame = 0;

        //10F���ς�FPS���v��
        std::array<double, 10> times;
        double time = 0, deltatime = 0;
        std::chrono::high_resolution_clock::time_point now, prev = std::chrono::high_resolution_clock::now();
        //�J�����̈ړ��X�s�[�h�A���W
        constexpr double speed = 20.f;
        glm::vec3 pos(0, 0, 10.f);
        double x = 0, y = 0;
        //��]�p�x
        float angle = 0;

        //�E�B���h�E�j���̒ʒm��������Esc�L�[�ŏI��
        while (!context.shouldClose() && !context.getKey(Key::Escape))
        {
            //���o�͍X�V
            if (Result::eSuccess != context.updateInput())
               assert(!"Failed to handle event!");

            {//�e����\��
                now = std::chrono::high_resolution_clock::now();
                time += times[frame % 10] = std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count() / 1000000.;
                deltatime = times[frame % 10];
                std::cerr << "now frame : " << frame << "\n";
                std::cerr << "fps : " << 1. / (std::accumulate(times.begin(), times.end(), 0.) / 10.) << "\n";
                std::cerr << "deltatime : " << static_cast<float>(deltatime) << "\n";
                context.getMousePos(window, x, y);
                std::cerr << "mouse x: " << x << " y: " << y << "\n";
            }

            {//�J�������ړ����Ă݂�
                if (context.getKey(Key::W))
                    pos.z -= speed * deltatime;
                if (context.getKey(Key::S))
                    pos.z += speed * deltatime;
                if (context.getKey(Key::A))
                    pos.x -= speed * deltatime;
                if (context.getKey(Key::D))
                    pos.x += speed * deltatime;
                if (context.getKey(Key::Up))
                    pos.y += speed * deltatime;
                if (context.getKey(Key::Down))
                    pos.y -= speed * deltatime;
            }

            {//UBO��������
                angle = static_cast<int>(angle + 80.f * static_cast<float>(deltatime)) % 360;
                Uniform ubo;
                ubo.world = glm::rotate(glm::identity<glm::mat4>(), glm::radians(angle), glm::vec3(0, 1.f, 0));
                ubo.view = glm::lookAtRH(pos, pos + glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
                ubo.proj = glm::perspective(glm::radians(45.f), 1.f * width / height, 1.f, 1000.f);
                ubo.proj[1][1] *= -1;

                uint32_t frameIndex = context.getFrameBufferIndex(presentPass);
                if (Result::eSuccess != context.writeBuffer(sizeof(Uniform), &ubo, renderUB))
                    assert(!"Failed to write uniform buffer!");
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGui::Begin("Hello, world!", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
            ImGui::Text("This is some useful text.");
            //float param = 10;
            ImGui::SliderFloat("english", &angle, 0, 360.f);
            if(ImGui::Button("test button", ImVec2(100, 100)))
            {
                break;
            }
            ImGui::End();
            ImGui::Render();

            context.updateCommandBuffer(presentCL, presentCB);

            //�R�}���h���s
            if (Result::eSuccess != context.execute(contourCB))
                assert(!"Failed to execute command!");
            if (Result::eSuccess != context.execute(renderCB))
                assert(!"Failed to execute command!");
            if (Result::eSuccess != context.execute(presentCB))
                assert(!"Failed to execute command!");
            {//�X�V
                ++frame;
                prev = now;
            }
        }
        std::cout << "time : " << time << "\n";
        std::cout << "ave : " << 1. / (time / (1. * frame)) << "\n";
    }

    //�j������, �f�X�g���N�^�ł��Ă΂�邪�����ł͖����I�ɍs���Ă���
    context.destroy();

    return 0;
}