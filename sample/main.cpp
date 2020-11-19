#include <Cutlass.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>

using namespace Cutlass;

//���_�^
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 UV;
};

//�萔�o�b�t�@�^
struct Uniform
{
    glm::mat4 world;
    glm::mat4 view;
    glm::mat4 proj;
};

int main()
{
    //�萔
    constexpr uint32_t frameCount = 3;
    constexpr uint32_t width = 800, height = 600;
    

    //-----------------------------------------�W�I���g����`
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


        //�R���e�L�X�g�擾
    Context& context = Context::getInstance();

    {//������
        InitializeInfo ii("test", true);
        if (Result::eSuccess != context.initialize(ii))
            std::cout << "Failed to Initialize!\n";
    }

    HWindow window;
    {//�E�B���h�E�쐬
        WindowInfo wi(width, height, frameCount, "CutlassTest");
        if (Result::eSuccess != context.createWindow(wi, window))
            std::cout << "Failed to create window!";
    }


    HBuffer vertexBuffer;
    {//���_�o�b�t�@�쐬, ��������
        BufferInfo bi;
        bi.setVertexBuffer<decltype(vertices[0])>(vertices.size());
        if (Result::eSuccess != context.createBuffer(bi, vertexBuffer))
            std::cout << "Failed to create vertex buffer!\n";
        if (Result::eSuccess != context.writeBuffer(bi.size, vertices.data(), vertexBuffer))
            std::cout << "Failed to write vertex buffer!\n";
    }

    HBuffer indexBuffer;
    {//�C���f�b�N�X�o�b�t�@�쐬, ��������
        BufferInfo bi;
        bi.setIndexBuffer<decltype(indices[0])>(indices.size());
        if (Result::eSuccess != context.createBuffer(bi, indexBuffer))
            std::cout << "Failed to create index buffer!\n";
        if (Result::eSuccess != context.writeBuffer(bi.size, indices.data(), indexBuffer))
            std::cout << "Failed to write index buffer!\n";
    }

    std::vector<HBuffer> uniformBuffers(frameCount);
    {//���j�t�H�[���o�b�t�@�쐬
        BufferInfo bi;
        bi.setUniformBuffer<Uniform>();
        for (auto& ub : uniformBuffers)
            if (Result::eSuccess != context.createBuffer(bi, ub))
                std::cout << "Failed to create uniform\n";
    }

    HTexture texture;
    {//�e�N�X�`���쐬
        if (Result::eSuccess != context.createTextureFromFile("./Textures/texture.png", texture))
            std::cout << "Failed to create texture from file!\n";
    }

    HTexture target;
    {//�`���p�e�N�X�`���쐬
        TextureInfo ti;
        ti.setRTTex2D(width, height, ResourceType::eUNormVec4);
        if (Result::eSuccess != context.createTexture(ti, target))
            std::cout << "Failed to create render target texture!\n";
    }

    HRenderDST texDST;
    {//�e�N�X�`���p�`���I�u�W�F�N�g�쐬
        if (Result::eSuccess != context.createRenderDST({ target }, texDST))
            std::cout << "Failed to create texture renderDST\n";
    }

    HRenderDST renderDST;
    {//�E�B���h�E�p�`���I�u�W�F�N�g�쐬
        if (Result::eSuccess != context.createRenderDST(window, true, renderDST))
            std::cout << "Failed to create renderDST\n";
    }

    HRenderPipeline renderPipeline1, renderPipeline2;
    {//�e�N�X�`���`��p�p�X�A�E�B���h�E�`��p�p�X���`

        //���_���C�A�E�g��`
        VertexLayout vl;
        vl.set(ResourceType::eF32Vec3, "pos");
        vl.set(ResourceType::eF32Vec3, "color");
        vl.set(ResourceType::eF32Vec3, "normal");
        vl.set(ResourceType::eF32Vec2, "UV");

        //�V�F�[�_���\�[�X���C�A�E�g��`
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
            Shader("./Shaders/vert.spv", "main"),
            Shader("./Shaders/frag.spv", "main"),
            SRDesc,
            texDST
        );

        if (Result::eSuccess != context.createRenderPipeline(rpi, renderPipeline1))
            std::cout << "Failed to create render pipeline!\n";

        //2�p�X�ڂ͑Ώۂƃf�v�X�o�b�t�@�����O���ς��
        rpi.depthStencilState = DepthStencilState::eDepth;
        rpi.renderDST = renderDST;

        if (Result::eSuccess != context.createRenderPipeline(rpi, renderPipeline2))
            std::cout << "Failed to create render pipeline2!\n";
    }

    std::vector<ShaderResourceSet> Sets1(frameCount);
    {//1�p�X�ڂ̃��\�[�X�Z�b�g
        for (size_t i = 0; i < Sets1.size(); ++i)
        {
            Sets1[i].setUniformBuffer(0, uniformBuffers[i]);
            Sets1[i].setCombinedTexture(1, texture);
        }
    }

    std::vector<ShaderResourceSet> Sets2(frameCount);
    {//2�p�X�ڂ̃��\�[�X�Z�b�g
        for (size_t i = 0; i < Sets2.size(); ++i)
        {
            Sets2[i].setUniformBuffer(0, uniformBuffers[i]);
            Sets2[i].setCombinedTexture(1, target);
        }
    }

    std::vector<CommandList> commandLists(frameCount);
    {//�R�}���h���X�g���쐬
        ColorClearValue ccv{ 0, 0.5f, 0, 0 };
        DepthClearValue dcv(1.f, 0);
        for (size_t i = 0; i < commandLists.size(); ++i)
        {
            commandLists[i].beginRenderPipeline(renderPipeline1, ccv, dcv);
            commandLists[i].bindVB(vertexBuffer);
            commandLists[i].bindIB(indexBuffer);
            commandLists[i].bindSRSet(Sets1[i]);
            commandLists[i].renderIndexed(indices.size(), 1, 0, 0, 0);
            commandLists[i].endRenderPipeline();
            commandLists[i].sync();

            commandLists[i].beginRenderPipeline(renderPipeline2);
            commandLists[i].bindVB(vertexBuffer);
            commandLists[i].bindIB(indexBuffer);
            commandLists[i].bindSRSet(Sets2[i]);
            commandLists[i].renderIndexed(indices.size(), 1, 0, 0, 0);
            commandLists[i].endRenderPipeline();
            commandLists[i].present();
        }
    }

    HCommandBuffer commandBuffer;
    {//���X�g����GPU�Ńo�b�t�@���\�z
        if (Result::eSuccess != context.createCommandBuffer(commandLists, commandBuffer))
            std::cout << "Failed to create command buffer\n";
    }

    {//���C�����[�v

        int frame = 0;
        Event event;

        //10F���ς�FPS���v��
        std::array<double, 10> times;
        std::chrono::high_resolution_clock::time_point now, prev = std::chrono::high_resolution_clock::now();
        //�J�����̈ړ��X�s�[�h�A���W
        constexpr double speed = 0.5f;
        glm::vec3 pos(0, 0, 10.f);

        //�E�B���h�E�j���̒ʒm��������Esc�L�[�ŏI��
        while (!event.windowShouldClose() && event.getKeyState(Key::Escape) != KeyState::ePressed)
        {
            {//�e����\��
                now = std::chrono::high_resolution_clock::now();
                times[frame % 10] = std::chrono::duration_cast<std::chrono::microseconds>((now - prev)).count() / 1000000.;
                std::cout << "now frame : " << frame++ << "\n";
                std::cout << "fps : " << 1. / (std::accumulate(times.begin(), times.end(), 0.) / 10.) << "\n";
            }

            {//�J�������ړ����Ă݂�
                if (event.getKeyFrame(Key::W))
                    pos.z -= speed;
                if (event.getKeyFrame(Key::S))
                    pos.z += speed;
                if (event.getKeyFrame(Key::A))
                    pos.x -= speed;
                if (event.getKeyFrame(Key::D))
                    pos.x += speed;
                if (event.getKeyFrame(Key::Up))
                    pos.y += speed;
                if (event.getKeyFrame(Key::Down))
                    pos.y -= speed;
            }

            {//UBO��������
                Uniform ubo;
                ubo.world = glm::rotate(glm::identity<glm::mat4>(), glm::radians(2.f * frame), glm::vec3(0, 1.f, 0));
                ubo.view = glm::lookAtRH(pos, pos + glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
                ubo.proj = glm::perspective(glm::radians(45.f), 1.f * width / height, 1.f, 100.f);
                ubo.proj[1][1] *= -1;

                uint32_t frameIndex = context.getFrameBufferIndex(renderDST);
                if (Result::eSuccess != context.writeBuffer(sizeof(Uniform), &ubo, uniformBuffers[(frameIndex + frameCount - 1) % frameCount]))
                    std::cout << "Failed to write uniform buffer!\n";
            }

            if (Result::eSuccess != context.handleEvent(window, event))
                std::cerr << "event handling failed!\n";

            if (Result::eSuccess != context.execute(commandBuffer))
                std::cerr << "Failed to execute command!\n";

            prev = now;
        }
    }

    //�j�������A�����I�ɍs���Ă��邪���[�U���s��Ȃ��Ă��悢
    context.destroy();

    return 0;
}