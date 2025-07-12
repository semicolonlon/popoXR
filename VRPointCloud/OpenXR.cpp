#include "OpenXR.h"

void OpenXR::initializeGL() {

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    std::string vertexSource = ReadTextFile("VertexShader.glsl");
    m_vertexShader = m_graphicsAPI->CreateShader({ GraphicsAPI::ShaderCreateInfo::Type::VERTEX, vertexSource.data(), vertexSource.size() });

    std::string fragmentSource = ReadTextFile("FragmentShader.glsl");
    m_fragmentShader = m_graphicsAPI->CreateShader({ GraphicsAPI::ShaderCreateInfo::Type::FRAGMENT, fragmentSource.data(), fragmentSource.size() });

    GraphicsAPI::PipelineCreateInfo pipelineCI;
    pipelineCI.shaders = { m_vertexShader, m_fragmentShader };
    pipelineCI.vertexInputState.attributes = { {0, 0, GraphicsAPI::VertexType::VEC4, 0, "TEXCOORD"} };
    pipelineCI.vertexInputState.bindings = { {0, 0, 4 * sizeof(float)} };
    pipelineCI.inputAssemblyState = { GraphicsAPI::PrimitiveTopology::TRIANGLE_LIST, false };
    pipelineCI.rasterisationState = { false, false, GraphicsAPI::PolygonMode::FILL, GraphicsAPI::CullMode::BACK, GraphicsAPI::FrontFace::COUNTER_CLOCKWISE, false, 0.0f, 0.0f, 0.0f, 1.0f };
    pipelineCI.multisampleState = { 1, false, 1.0f, 0xFFFFFFFF, false, false };
    pipelineCI.depthStencilState = { true, true, GraphicsAPI::CompareOp::LESS_OR_EQUAL, false, false, {}, {}, 0.0f, 1.0f };
    pipelineCI.colorBlendState = { false, GraphicsAPI::LogicOp::NO_OP, {{true, GraphicsAPI::BlendFactor::SRC_ALPHA, GraphicsAPI::BlendFactor::ONE_MINUS_SRC_ALPHA, GraphicsAPI::BlendOp::ADD, GraphicsAPI::BlendFactor::ONE, GraphicsAPI::BlendFactor::ZERO, GraphicsAPI::BlendOp::ADD, (GraphicsAPI::ColorComponentBit)15}}, {0.0f, 0.0f, 0.0f, 0.0f} };
    pipelineCI.colorFormats = { m_colorSwapchainInfos[0].swapchainFormat };
    pipelineCI.depthFormat = m_depthSwapchainInfos[0].swapchainFormat;
    pipelineCI.layout = { {0, nullptr, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::VERTEX},
                         {1, nullptr, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::VERTEX},
                         {2, nullptr, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::FRAGMENT} };
    m_pipeline = m_graphicsAPI->CreatePipeline(pipelineCI);

}

void OpenXR::destroyGL() {
    m_graphicsAPI->DestroyPipeline(m_pipeline);
    m_graphicsAPI->DestroyShader(m_fragmentShader);
    m_graphicsAPI->DestroyShader(m_vertexShader);
    m_graphicsAPI->DestroyBuffer(m_uniformBuffer_Camera);
    m_graphicsAPI->DestroyBuffer(m_uniformBuffer_Normals);
    m_graphicsAPI->DestroyBuffer(m_indexBuffer);
    m_graphicsAPI->DestroyBuffer(m_vertexBuffer);
}