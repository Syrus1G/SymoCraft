#pragma once
#include "core.h"
#include "world/world.h"

namespace SymoCraft{

    // Batch Procedure:
    // I.Initialization
    //  1. Create and bind buffers (full DSA)
    //  2. Allocate maximum batch memory to the VBO
    //  3. Configure vertex attributes
    // II.Feed 'data' with vertices
    //   1. Check errors
    //   2. Feed 'data'
    //   3. Tick index
    // III.Draw all vertices in the memory
    //   1. Deliver data to VBO memory
    //   2. Draw vertices
    //   3. Clear the batch

    struct Vertex3D{
        glm::ivec3 pos_coord;
        glm::vec3 tex_coord;
        float normal;
    };

    struct VertexAttribute{
        uint16 attribute_slot;
        uint16 element_amount;
        GLenum data_type;
        uint16 offset;
    };

    struct DrawArraysIndirectCommand
    {
        uint16  count;
        uint16  instanceCount;
        uint16  first;
        uint16  baseInstance;
    };

    inline constexpr uint32 kMaxBatchSize = 10000000;

    template<typename T>
    class Batch
    {
    public:
        void init(std::initializer_list<VertexAttribute> vertex_attributes)
        {
            m_data_size = sizeof(T) * kMaxBatchSize;
            data = (T*)AmoMemory_Allocate(m_data_size);
            m_vertex_count = 0;
            zIndex = 0;

            // Create buffers
            glCreateBuffers(1, &m_vertex_data_vbo);
            glCreateBuffers(1, &m_draw_command_vbo);
            glCreateVertexArrays(1, &m_vao);

            // Allocate memory for the VBO, and bind the buffers
            glNamedBufferStorage(m_vertex_data_vbo, m_data_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
            glNamedBufferStorage(m_draw_command_vbo, World::chunk_radius * World::chunk_radius, nullptr, GL_DYNAMIC_DRAW);
            glVertexArrayVertexBuffer(m_vao, 0, m_vertex_data_vbo, 0, sizeof(Vertex3D));


            // Configure vertex attributes
            // Draw float data solely for now
            // Add support for other data in the future
            for (const auto& attribute : vertex_attributes)
            {
                glEnableVertexArrayAttrib(m_vao, attribute.attribute_slot);
                glVertexArrayAttribFormat(m_vao, attribute.attribute_slot, attribute.element_amount, attribute.data_type, GL_FALSE, attribute.offset);
                glVertexArrayAttribBinding(m_vao, attribute.attribute_slot, 0);
            }

        }

        void AddVertex(const T& vertex)
        {
            if(!data)
                AmoLogger_Error("Invalid batch.\n");
            if (!hasRoom())
            {
                AmoLogger_Error("Batch ran out of room. I have %d/%d vertices.\n", m_vertex_count, kMaxBatchSize);
                return;
            }
            if (m_vertex_count < 0)
            {
                AmoLogger_Error("Invalid vertex number.\n");
                return;
            }

            data[m_vertex_count] = vertex;
            m_vertex_count++;
        }

        void AddVertex(const T* vertex, uint32 vertex_amount)
        {
            if(!data)
                AmoLogger_Error("Invalid batch.\n");
            if (!hasRoom())
            {
                AmoLogger_Error("Batch ran out of room. I have %d/%d vertices.\n", m_vertex_count, kMaxBatchSize);
                return;
            }
            if (m_vertex_count < 0)
            {
                AmoLogger_Error("Invalid vertex number.\n");
                return;
            }

            glNamedBufferSubData(m_vertex_data_vbo, m_vertex_count * sizeof(Vertex3D), vertex_amount * sizeof(Vertex3D), vertex);
            m_vertex_count += vertex_amount;
        }

        void Draw()  //Draw vertices
        {
            if (m_vertex_count <= 0)
            {
                std::cerr << "No vertices to draw.\n";
                return;
            }

            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLES, 0, m_vertex_count);
            glBindVertexArray(0);

            Clear();
        }

        inline void ReloadData()
        {
            glNamedBufferSubData(m_vertex_data_vbo, 0, m_data_size, data);
        }

        inline void Clear()
        {
            m_vertex_count = 0;
        }

        inline void Free()
        {
            if (data)
            {
                AmoMemory_Free(data);
                data = nullptr;
                m_data_size = 0;
            }
        }

        inline bool operator<(const Batch& batch) const
        {
            return (zIndex < batch.zIndex);
        }

    private:
        uint32 m_vao;
        uint32 m_vertex_data_vbo;
        uint32 m_draw_command_vbo;
        uint32 m_data_size;
        uint32 m_vertex_count;
        int32 zIndex;
        T* data;

        inline bool hasRoom() const
        {
            return m_vertex_count <= kMaxBatchSize;
        }
    };

}