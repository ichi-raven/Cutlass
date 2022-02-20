#pragma once

#include <cstdint>
#include <unordered_map>

namespace Cutlass
{
    // using HWindow           = uint16_t;
    // using HBuffer           = uint32_t;
    // using HTexture          = uint32_t;
    // using HSampler          = uint32_t;
    // using HRenderPass       = uint32_t;
    // using HGraphicsPipeline = uint32_t;
    // using HCommandBuffer    = uint32_t;

    struct HWindow
    {
        bool operator==(const HWindow& r) const
        {
            return id == r.id;
        }

        bool operator!=(const HWindow& r) const
        {
            return id != r.id;
        }

        HWindow& operator++()
        {
            ++id;
            return *this;
        }

        HWindow operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        HWindow& operator--()
        {
            --id;
            return *this;
        }

        HWindow operator--(int)
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        uint16_t setID(uint16_t rid)
        {
            id = rid;
            return rid;
        }

        uint16_t getID() const
        {
            return id;
        }

    private:
        uint16_t id;
    };

    struct HBuffer
    {
        bool operator==(const HBuffer& r) const
        {
            return id == r.id;
        }

        bool operator!=(const HBuffer& r) const
        {
            return id != r.id;
        }

        HBuffer& operator++()
        {
            ++id;
            return *this;
        }

        HBuffer operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        HBuffer& operator--()
        {
            --id;
            return *this;
        }

        HBuffer operator--(int)
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        uint32_t setID(uint32_t rid)
        {
            id = rid;
            return rid;
        }

        uint32_t getID() const
        {
            return id;
        }

    private:
        uint32_t id;
    };

    struct HTexture
    {

        bool operator==(const HTexture& r) const
        {
            return id == r.id;
        }

        bool operator!=(const HTexture& r) const
        {
            return id != r.id;
        }

        HTexture& operator++()
        {
            ++id;
            return *this;
        }

        HTexture operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        HTexture& operator--()
        {
            --id;
            return *this;
        }

        HTexture operator--(int)
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        uint32_t setID(uint32_t rid)
        {
            id = rid;
            return rid;
        }

        uint32_t getID() const
        {
            return id;
        }

    private:
        uint32_t id;
    };

    struct HRenderPass
    {

        bool operator==(const HRenderPass& r) const
        {
            return id == r.id;
        }

        bool operator!=(const HRenderPass& r) const
        {
            return id != r.id;
        }

        HRenderPass& operator++()
        {
            ++id;
            return *this;
        }

        HRenderPass operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        HRenderPass& operator--()
        {
            --id;
            return *this;
        }

        HRenderPass operator--(int)
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        uint32_t setID(uint32_t rid)
        {
            id = rid;
            return rid;
        }

        uint32_t getID() const
        {
            return id;
        }

    private:
        uint32_t id;
    };

    struct HGraphicsPipeline
    {
        bool operator==(const HGraphicsPipeline& r) const
        {
            return id == r.id;
        }

        bool operator!=(const HGraphicsPipeline& r) const
        {
            return id != r.id;
        }

        HGraphicsPipeline& operator++()
        {
            ++id;
            return *this;
        }

        HGraphicsPipeline operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        HGraphicsPipeline& operator--()
        {
            --id;
            return *this;
        }

        HGraphicsPipeline operator--(int)
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        uint32_t setID(uint32_t rid)
        {
            id = rid;
            return rid;
        }

        uint32_t getID() const
        {
            return id;
        }

    private:
        uint32_t id;
    };

    struct HCommandBuffer
    {

        bool operator==(const HCommandBuffer& r) const
        {
            return id == r.id;
        }

        bool operator!=(const HCommandBuffer& r) const
        {
            return id != r.id;
        }

        HCommandBuffer& operator++()
        {
            ++id;
            return *this;
        }

        HCommandBuffer operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        HCommandBuffer& operator--()
        {
            --id;
            return *this;
        }

        HCommandBuffer operator--(int)
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        uint32_t setID(uint32_t rid)
        {
            id = rid;
            return rid;
        }
        
        uint32_t getID() const
        {
            return id;
        }

    private:
        uint32_t id;
    };

    enum class Result
    {
        eFailure = 0,
        eSuccess,
    };

    enum class ResourceType  //とりあえずglmを想定している
    {
        eUint32 = 0,
        eInt32,      // 1
        eUnorm,      // 2
        eFloat32,    // 3
        eF32Vec2,    // 4
        eF32Vec3,    // 5
        eF32Vec4,    // 6
        eU32Vec2,    // 7
        eU32Vec3,    // 8
        eU32Vec4,    // 9
        eS32Vec2,    // 10
        eS32Vec3,    // 11
        eS32Vec4,    // 12
        eUNormVec2,  // 13
        eUNormVec3,  // 14
        eUNormVec4,  // 15
        eFMat2,      // 16
        eFMat3,      // 17
        eFMat4,      // 18
    };
};  // namespace Cutlass

namespace std
{
    template <>
    struct hash<Cutlass::HWindow>
    {
        size_t operator()(const Cutlass::HWindow& data) const
        {
            return std::hash<uint16_t>()(data.getID());
        }
    };

    template <>
    struct hash<Cutlass::HBuffer>
    {
        size_t operator()(const Cutlass::HBuffer& data) const
        {
            return std::hash<uint32_t>()(data.getID());
        }
    };

    template <>
    struct hash<Cutlass::HTexture>
    {
        size_t operator()(const Cutlass::HTexture& data) const
        {
            return std::hash<uint32_t>()(data.getID());
        }
    };

    template <>
    struct hash<Cutlass::HRenderPass>
    {
        size_t operator()(const Cutlass::HRenderPass& data) const
        {
            return std::hash<uint32_t>()(data.getID());
        }
    };

    template <>
    struct hash<Cutlass::HGraphicsPipeline>
    {
        size_t operator()(const Cutlass::HGraphicsPipeline& data) const
        {
            return std::hash<uint32_t>()(data.getID());
        }
    };

    template <>
    struct hash<Cutlass::HCommandBuffer>
    {
        size_t operator()(const Cutlass::HCommandBuffer& data) const
        {
            return std::hash<uint32_t>()(data.getID());
        }
    };
}  // namespace std