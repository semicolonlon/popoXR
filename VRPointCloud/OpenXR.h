#include <DebugOutput.h>
#include <GraphicsAPI_OpenGL.h>
#include <OpenXRDebugUtils.h>
// OpenXRの線形代数ライブラリ
#include <xr_linear_algebra.h>

class OpenXR {
private:
    struct RenderLayerInfo;

public:
    OpenXR(GraphicsAPI_Type apiType)
        : m_apiType(apiType) {
        // グラフィックAPIの環境適合を確認
        if (!CheckGraphicsAPI_TypeIsValidForPlatform(m_apiType)) {
            XR_LOG_ERROR("ERROR: The provided Graphics API is not valid for this platform.");
            DEBUG_BREAK;
        }
    }
    ~OpenXR() = default;

    void Run() {
        CreateInstance();
        CreateDebugMessenger();

        GetInstanceProperties();
        GetSystemID();

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_1
        GetViewConfigurationViews();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_2
        GetEnvironmentBlendModes();
#endif

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_2_2
        CreateSession();
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_2
        CreateReferenceSpace();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_1
        CreateSwapchains();
#endif
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_3
        initializeGL();
#endif

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_2_3
        while (m_applicationRunning) {
            PollSystemEvents();
            PollEvents();
            if (m_sessionRunning) {
                RenderFrame();
            }
        }
#endif

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_1
        DestroySwapchains();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_2
        DestroyReferenceSpace();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_3
        destroyGL();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_2_2
        DestroySession();
#endif

        DestroyDebugMessenger();
        DestroyInstance();
    }

private:
    void CreateInstance();
    void DestroyInstance();
    void CreateDebugMessenger();       // ログの受け取りに関する設定
    void DestroyDebugMessenger();
    void GetInstanceProperties();      // OpenXR ランタイムに関する情報の出力
    void GetSystemID();
    void GetEnvironmentBlendModes();   // 現実世界との融合に関する   (real     : 0.0%)
    void GetViewConfigurationViews();  // ビュー構成タイプの選択     (VienType : STEREO)
    void CreateSession();              //グラフィックスAPIの指定     (API      : OpenGL)
    void DestroySession();

    struct CameraConstants {
        XrMatrix4x4f viewProj;
        XrMatrix4x4f modelViewProj;
        XrMatrix4x4f model;
        XrVector4f color;
        XrVector4f pad1;
        XrVector4f pad2;
        XrVector4f pad3;
    };

    CameraConstants cameraConstants;
    XrVector4f normals[6] = {
        {1.00f, 0.00f, 0.00f, 0},
        {-1.00f, 0.00f, 0.00f, 0},
        {0.00f, 1.00f, 0.00f, 0},
        {0.00f, -1.00f, 0.00f, 0},
        {0.00f, 0.00f, 1.00f, 0},
        {0.00f, 0.0f, -1.00f, 0} };

    void initializeGL();
    void destroyGL();

    void PollEvents();                 // OpenXRの状態をポーリング(監視)

    // 原点の設定
    void CreateReferenceSpace() {
        // 原点設定のモード  (mode   :   LOCAL)
        // LOCAL : 立ち位置が基準、VIEW : ヘッドセットの位置、STAGE : 物理的な床
        XrReferenceSpaceCreateInfo referenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        // 回転、移動
        referenceSpaceCI.poseInReferenceSpace = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} };
        OPENXR_CHECK(xrCreateReferenceSpace(m_session, &referenceSpaceCI, &m_localSpace), "Failed to create ReferenceSpace.");
    }

    void DestroyReferenceSpace() {
        // Destroy the reference XrSpace.
        OPENXR_CHECK(xrDestroySpace(m_localSpace), "Failed to destroy Space.")
    }

    void CreateSwapchains() {
        // Get the supported swapchain formats as an array of int64_t and ordered by runtime preference.
        uint32_t formatCount = 0;
        OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr), "Failed to enumerate Swapchain Formats");
        std::vector<int64_t> formats(formatCount);
        OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount, formats.data()), "Failed to enumerate Swapchain Formats");
        if (m_graphicsAPI->SelectDepthSwapchainFormat(formats) == 0) {
            std::cerr << "Failed to find depth format for Swapchain." << std::endl;
            DEBUG_BREAK;
        }

        //Resize the SwapchainInfo to match the number of view in the View Configuration.
        m_colorSwapchainInfos.resize(m_viewConfigurationViews.size());
        m_depthSwapchainInfos.resize(m_viewConfigurationViews.size());

        // Per view, create a color and depth swapchain, and their associated image views.
        for (size_t i = 0; i < m_viewConfigurationViews.size(); i++) {
            SwapchainInfo& colorSwapchainInfo = m_colorSwapchainInfos[i];
            SwapchainInfo& depthSwapchainInfo = m_depthSwapchainInfos[i];

            // Fill out an XrSwapchainCreateInfo structure and create an XrSwapchain.
            // Color.
            XrSwapchainCreateInfo swapchainCI{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
            swapchainCI.createFlags = 0;
            swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            swapchainCI.format = m_graphicsAPI->SelectColorSwapchainFormat(formats);                // Use GraphicsAPI to select the first compatible format.
            swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;  // Use the recommended values from the XrViewConfigurationView.
            swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
            swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
            swapchainCI.faceCount = 1;
            swapchainCI.arraySize = 1;
            swapchainCI.mipCount = 1;
            OPENXR_CHECK(xrCreateSwapchain(m_session, &swapchainCI, &colorSwapchainInfo.swapchain), "Failed to create Color Swapchain");
            colorSwapchainInfo.swapchainFormat = swapchainCI.format;  // Save the swapchain format for later use.

            // Depth.
            swapchainCI.createFlags = 0;
            swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            swapchainCI.format = m_graphicsAPI->SelectDepthSwapchainFormat(formats);                // Use GraphicsAPI to select the first compatible format.
            swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;  // Use the recommended values from the XrViewConfigurationView.
            swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
            swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
            swapchainCI.faceCount = 1;
            swapchainCI.arraySize = 1;
            swapchainCI.mipCount = 1;
            OPENXR_CHECK(xrCreateSwapchain(m_session, &swapchainCI, &depthSwapchainInfo.swapchain), "Failed to create Depth Swapchain");
            depthSwapchainInfo.swapchainFormat = swapchainCI.format;  // Save the swapchain format for later use.

            // Get the number of images in the color/depth swapchain and allocate Swapchain image data via GraphicsAPI to store the returned array.
            uint32_t colorSwapchainImageCount = 0;
            OPENXR_CHECK(xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, 0, &colorSwapchainImageCount, nullptr), "Failed to enumerate Color Swapchain Images.");
            XrSwapchainImageBaseHeader* colorSwapchainImages = m_graphicsAPI->AllocateSwapchainImageData(colorSwapchainInfo.swapchain, GraphicsAPI::SwapchainType::COLOR, colorSwapchainImageCount);
            OPENXR_CHECK(xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, colorSwapchainImageCount, &colorSwapchainImageCount, colorSwapchainImages), "Failed to enumerate Color Swapchain Images.");

            uint32_t depthSwapchainImageCount = 0;
            OPENXR_CHECK(xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, 0, &depthSwapchainImageCount, nullptr), "Failed to enumerate Depth Swapchain Images.");
            XrSwapchainImageBaseHeader* depthSwapchainImages = m_graphicsAPI->AllocateSwapchainImageData(depthSwapchainInfo.swapchain, GraphicsAPI::SwapchainType::DEPTH, depthSwapchainImageCount);
            OPENXR_CHECK(xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, depthSwapchainImageCount, &depthSwapchainImageCount, depthSwapchainImages), "Failed to enumerate Depth Swapchain Images.");

            // Per image in the swapchains, fill out a GraphicsAPI::ImageViewCreateInfo structure and create a color/depth image view.
            for (uint32_t j = 0; j < colorSwapchainImageCount; j++) {
                GraphicsAPI::ImageViewCreateInfo imageViewCI;
                imageViewCI.image = m_graphicsAPI->GetSwapchainImage(colorSwapchainInfo.swapchain, j);
                imageViewCI.type = GraphicsAPI::ImageViewCreateInfo::Type::RTV;
                imageViewCI.view = GraphicsAPI::ImageViewCreateInfo::View::TYPE_2D;
                imageViewCI.format = colorSwapchainInfo.swapchainFormat;
                imageViewCI.aspect = GraphicsAPI::ImageViewCreateInfo::Aspect::COLOR_BIT;
                imageViewCI.baseMipLevel = 0;
                imageViewCI.levelCount = 1;
                imageViewCI.baseArrayLayer = 0;
                imageViewCI.layerCount = 1;
                colorSwapchainInfo.imageViews.push_back(m_graphicsAPI->CreateImageView(imageViewCI));
            }
            for (uint32_t j = 0; j < depthSwapchainImageCount; j++) {
                GraphicsAPI::ImageViewCreateInfo imageViewCI;
                imageViewCI.image = m_graphicsAPI->GetSwapchainImage(depthSwapchainInfo.swapchain, j);
                imageViewCI.type = GraphicsAPI::ImageViewCreateInfo::Type::DSV;
                imageViewCI.view = GraphicsAPI::ImageViewCreateInfo::View::TYPE_2D;
                imageViewCI.format = depthSwapchainInfo.swapchainFormat;
                imageViewCI.aspect = GraphicsAPI::ImageViewCreateInfo::Aspect::DEPTH_BIT;
                imageViewCI.baseMipLevel = 0;
                imageViewCI.levelCount = 1;
                imageViewCI.baseArrayLayer = 0;
                imageViewCI.layerCount = 1;
                depthSwapchainInfo.imageViews.push_back(m_graphicsAPI->CreateImageView(imageViewCI));
            }
        }
    }

    void DestroySwapchains() {
        // Per view in the view configuration:
        for (size_t i = 0; i < m_viewConfigurationViews.size(); i++) {
            SwapchainInfo& colorSwapchainInfo = m_colorSwapchainInfos[i];
            SwapchainInfo& depthSwapchainInfo = m_depthSwapchainInfos[i];

            // Destroy the color and depth image views from GraphicsAPI.
            for (void*& imageView : colorSwapchainInfo.imageViews) {
                m_graphicsAPI->DestroyImageView(imageView);
            }
            for (void*& imageView : depthSwapchainInfo.imageViews) {
                m_graphicsAPI->DestroyImageView(imageView);
            }

            // Free the Swapchain Image Data.
            m_graphicsAPI->FreeSwapchainImageData(colorSwapchainInfo.swapchain);
            m_graphicsAPI->FreeSwapchainImageData(depthSwapchainInfo.swapchain);

            // Destroy the swapchains.
            OPENXR_CHECK(xrDestroySwapchain(colorSwapchainInfo.swapchain), "Failed to destroy Color Swapchain");
            OPENXR_CHECK(xrDestroySwapchain(depthSwapchainInfo.swapchain), "Failed to destroy Depth Swapchain");
        }
    }

    size_t renderCuboidIndex = 0;
    void RenderCuboid(XrPosef pose, XrVector3f scale, XrVector3f color) {
        XrMatrix4x4f_CreateTranslationRotationScale(&cameraConstants.model, &pose.position, &pose.orientation, &scale);

        XrMatrix4x4f_Multiply(&cameraConstants.modelViewProj, &cameraConstants.viewProj, &cameraConstants.model);
        cameraConstants.color = { color.x, color.y, color.z, 1.0 };
        size_t offsetCameraUB = sizeof(CameraConstants) * renderCuboidIndex;

        m_graphicsAPI->SetPipeline(m_pipeline);

        m_graphicsAPI->SetBufferData(m_uniformBuffer_Camera, offsetCameraUB, sizeof(CameraConstants), &cameraConstants);
        m_graphicsAPI->SetDescriptor({ 0, m_uniformBuffer_Camera, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::VERTEX, false, offsetCameraUB, sizeof(CameraConstants) });
        m_graphicsAPI->SetDescriptor({ 1, m_uniformBuffer_Normals, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::VERTEX, false, 0, sizeof(normals) });

        m_graphicsAPI->UpdateDescriptors();

        m_graphicsAPI->SetVertexBuffers(&m_vertexBuffer, 1);
        m_graphicsAPI->SetIndexBuffer(m_indexBuffer);
        m_graphicsAPI->DrawIndexed(36);

        renderCuboidIndex++;
    }

    void RenderFrame() {
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_2
        // Get the XrFrameState for timing and rendering info.
        XrFrameState frameState{ XR_TYPE_FRAME_STATE };
        XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
        OPENXR_CHECK(xrWaitFrame(m_session, &frameWaitInfo, &frameState), "Failed to wait for XR Frame.");

        // Tell the OpenXR compositor that the application is beginning the frame.
        XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
        OPENXR_CHECK(xrBeginFrame(m_session, &frameBeginInfo), "Failed to begin the XR Frame.");

        // Variables for rendering and layer composition.
        bool rendered = false;
        RenderLayerInfo renderLayerInfo;
        renderLayerInfo.predictedDisplayTime = frameState.predictedDisplayTime;

        // Check that the session is active and that we should render.
        bool sessionActive = (m_sessionState == XR_SESSION_STATE_SYNCHRONIZED || m_sessionState == XR_SESSION_STATE_VISIBLE || m_sessionState == XR_SESSION_STATE_FOCUSED);
        if (sessionActive && frameState.shouldRender) {
            // Render the stereo image and associate one of swapchain images with the XrCompositionLayerProjection structure.
            rendered = RenderLayer(renderLayerInfo);
            if (rendered) {
                renderLayerInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&renderLayerInfo.layerProjection));
            }
        }

        // Tell OpenXR that we are finished with this frame; specifying its display time, environment blending and layers.
        XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
        frameEndInfo.displayTime = frameState.predictedDisplayTime;
        frameEndInfo.environmentBlendMode = m_environmentBlendMode;
        frameEndInfo.layerCount = static_cast<uint32_t>(renderLayerInfo.layers.size());
        frameEndInfo.layers = renderLayerInfo.layers.data();
        OPENXR_CHECK(xrEndFrame(m_session, &frameEndInfo), "Failed to end the XR Frame.");
#endif
    }

    bool RenderLayer(RenderLayerInfo& renderLayerInfo) {
        // Locate the views from the view configuration within the (reference) space at the display time.
        std::vector<XrView> views(m_viewConfigurationViews.size(), { XR_TYPE_VIEW });

        XrViewState viewState{ XR_TYPE_VIEW_STATE };  // Will contain information on whether the position and/or orientation is valid and/or tracked.
        XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
        viewLocateInfo.viewConfigurationType = m_viewConfiguration;
        viewLocateInfo.displayTime = renderLayerInfo.predictedDisplayTime;
        viewLocateInfo.space = m_localSpace;
        uint32_t viewCount = 0;
        XrResult result = xrLocateViews(m_session, &viewLocateInfo, &viewState, static_cast<uint32_t>(views.size()), &viewCount, views.data());
        if (result != XR_SUCCESS) {
            XR_LOG("Failed to locate Views.");
            return false;
        }

        // Resize the layer projection views to match the view count. The layer projection views are used in the layer projection.
        renderLayerInfo.layerProjectionViews.resize(viewCount, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });

        // Per view in the view configuration:
        for (uint32_t i = 0; i < viewCount; i++) {
            SwapchainInfo& colorSwapchainInfo = m_colorSwapchainInfos[i];
            SwapchainInfo& depthSwapchainInfo = m_depthSwapchainInfos[i];

            // Acquire and wait for an image from the swapchains.
            // Get the image index of an image in the swapchains.
            // The timeout is infinite.
            uint32_t colorImageIndex = 0;
            uint32_t depthImageIndex = 0;
            XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
            OPENXR_CHECK(xrAcquireSwapchainImage(colorSwapchainInfo.swapchain, &acquireInfo, &colorImageIndex), "Failed to acquire Image from the Color Swapchian");
            OPENXR_CHECK(xrAcquireSwapchainImage(depthSwapchainInfo.swapchain, &acquireInfo, &depthImageIndex), "Failed to acquire Image from the Depth Swapchian");

            XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
            waitInfo.timeout = XR_INFINITE_DURATION;
            OPENXR_CHECK(xrWaitSwapchainImage(colorSwapchainInfo.swapchain, &waitInfo), "Failed to wait for Image from the Color Swapchain");
            OPENXR_CHECK(xrWaitSwapchainImage(depthSwapchainInfo.swapchain, &waitInfo), "Failed to wait for Image from the Depth Swapchain");

            // Get the width and height and construct the viewport and scissors.
            const uint32_t& width = m_viewConfigurationViews[i].recommendedImageRectWidth;
            const uint32_t& height = m_viewConfigurationViews[i].recommendedImageRectHeight;
            GraphicsAPI::Viewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
            GraphicsAPI::Rect2D scissor = { {(int32_t)0, (int32_t)0}, {width, height} };
            float nearZ = 0.05f;
            float farZ = 100.0f;

            // Fill out the XrCompositionLayerProjectionView structure specifying the pose and fov from the view.
            // This also associates the swapchain image with this layer projection view.
            renderLayerInfo.layerProjectionViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
            renderLayerInfo.layerProjectionViews[i].pose = views[i].pose;
            renderLayerInfo.layerProjectionViews[i].fov = views[i].fov;
            renderLayerInfo.layerProjectionViews[i].subImage.swapchain = colorSwapchainInfo.swapchain;
            renderLayerInfo.layerProjectionViews[i].subImage.imageRect.offset.x = 0;
            renderLayerInfo.layerProjectionViews[i].subImage.imageRect.offset.y = 0;
            renderLayerInfo.layerProjectionViews[i].subImage.imageRect.extent.width = static_cast<int32_t>(width);
            renderLayerInfo.layerProjectionViews[i].subImage.imageRect.extent.height = static_cast<int32_t>(height);
            renderLayerInfo.layerProjectionViews[i].subImage.imageArrayIndex = 0;  // Useful for multiview rendering.

            // Rendering code to clear the color and depth image views.
            m_graphicsAPI->BeginRendering();

            if (m_environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
                // VR mode use a background color.
                m_graphicsAPI->ClearColor(colorSwapchainInfo.imageViews[colorImageIndex], 0.17f, 0.17f, 0.17f, 1.00f);
            }
            else {
                // In AR mode make the background color black.
                m_graphicsAPI->ClearColor(colorSwapchainInfo.imageViews[colorImageIndex], 0.00f, 0.00f, 0.00f, 1.00f);
            }
            m_graphicsAPI->ClearDepth(depthSwapchainInfo.imageViews[depthImageIndex], 1.0f);

            m_graphicsAPI->SetRenderAttachments(&colorSwapchainInfo.imageViews[colorImageIndex], 1, depthSwapchainInfo.imageViews[depthImageIndex], width, height, m_pipeline);
            m_graphicsAPI->SetViewports(&viewport, 1);
            m_graphicsAPI->SetScissors(&scissor, 1);

            // Compute the view-projection transform.
            // All matrices (including OpenXR's) are column-major, right-handed.
            XrMatrix4x4f proj;
            XrMatrix4x4f_CreateProjectionFov(&proj, m_apiType, views[i].fov, nearZ, farZ);
            XrMatrix4x4f toView;
            XrVector3f scale1m{ 1.0f, 1.0f, 1.0f };
            XrMatrix4x4f_CreateTranslationRotationScale(&toView, &views[i].pose.position, &views[i].pose.orientation, &scale1m);
            XrMatrix4x4f view;
            XrMatrix4x4f_InvertRigidBody(&view, &toView);
            XrMatrix4x4f_Multiply(&cameraConstants.viewProj, &proj, &view);

            renderCuboidIndex = 0;
            // Draw a floor. Scale it by 2 in the X and Z, and 0.1 in the Y,
            RenderCuboid({ {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, -m_viewHeightM, 0.0f} }, { 2.0f, 0.1f, 2.0f }, { 0.4f, 0.5f, 0.5f });
            // Draw a "table".
            RenderCuboid({ {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, -m_viewHeightM + 0.9f, -0.7f} }, { 1.0f, 0.2f, 1.0f }, { 0.6f, 0.6f, 0.4f });

            m_graphicsAPI->EndRendering();

            // Give the swapchain image back to OpenXR, allowing the compositor to use the image.
            XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            OPENXR_CHECK(xrReleaseSwapchainImage(colorSwapchainInfo.swapchain, &releaseInfo), "Failed to release Image back to the Color Swapchain");
            OPENXR_CHECK(xrReleaseSwapchainImage(depthSwapchainInfo.swapchain, &releaseInfo), "Failed to release Image back to the Depth Swapchain");
        }

        // Fill out the XrCompositionLayerProjection structure for usage with xrEndFrame().
        renderLayerInfo.layerProjection.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
        renderLayerInfo.layerProjection.space = m_localSpace;
        renderLayerInfo.layerProjection.viewCount = static_cast<uint32_t>(renderLayerInfo.layerProjectionViews.size());
        renderLayerInfo.layerProjection.views = renderLayerInfo.layerProjectionViews.data();

        return true;
    }

    void PollSystemEvents() {
        return;
    }

private:

    XrInstance m_xrInstance = XR_NULL_HANDLE;
    std::vector<const char*> m_activeAPILayers = {};
    std::vector<const char*> m_activeInstanceExtensions = {};
    std::vector<std::string> m_apiLayers = {};
    std::vector<std::string> m_instanceExtensions = {};

    XrDebugUtilsMessengerEXT m_debugUtilsMessenger = XR_NULL_HANDLE;

    XrFormFactor m_formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId m_systemID = {};
    XrSystemProperties m_systemProperties = { XR_TYPE_SYSTEM_PROPERTIES };

    GraphicsAPI_Type m_apiType = UNKNOWN;
    std::unique_ptr<GraphicsAPI> m_graphicsAPI = nullptr;

    XrSession m_session = {};
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
    bool m_applicationRunning = true;
    bool m_sessionRunning = false;
    // 描画の要求(ステレオビュー、モノビュー)
    std::vector<XrViewConfigurationType> m_applicationViewConfigurations = { XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO };
    std::vector<XrViewConfigurationType> m_viewConfigurations;
    XrViewConfigurationType m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
    std::vector<XrViewConfigurationView> m_viewConfigurationViews;

    struct SwapchainInfo {
        XrSwapchain swapchain = XR_NULL_HANDLE;
        int64_t swapchainFormat = 0;
        std::vector<void*> imageViews;
    };
    std::vector<SwapchainInfo> m_colorSwapchainInfos = {};
    std::vector<SwapchainInfo> m_depthSwapchainInfos = {};

    std::vector<XrEnvironmentBlendMode> m_applicationEnvironmentBlendModes = { XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE };
    std::vector<XrEnvironmentBlendMode> m_environmentBlendModes = {};
    XrEnvironmentBlendMode m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;

    XrSpace m_localSpace = XR_NULL_HANDLE;

    struct RenderLayerInfo {
        XrTime predictedDisplayTime = 0;
        std::vector<XrCompositionLayerBaseHeader*> layers;
        XrCompositionLayerProjection layerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
    };

    // In STAGE space, viewHeightM should be 0. In LOCAL space, it should be offset downwards, below the viewer's initial position.
    float m_viewHeightM = 1.5f;

    // Vertex and index buffers: geometry for our cuboids.
    void* m_vertexBuffer = nullptr;
    void* m_indexBuffer = nullptr;
    // Camera values constant buffer for the shaders.
    void* m_uniformBuffer_Camera = nullptr;
    // The normals are stored in a uniform buffer to simplify our vertex geometry.
    void* m_uniformBuffer_Normals = nullptr;







    // We use only two shaders in this app.
    void* m_vertexShader = nullptr, * m_fragmentShader = nullptr;

    // The pipeline is a graphics-API specific state object.
    void* m_pipeline = nullptr;

    class Chunk {
    public:
        XrVector3f center;
        std::vector<GLuint*> vaos;
        std::vector<int> pointNum;
        
        ~Chunk() {
            center.x = 0.0f;
            center.y = 0.0f;
            center.z = 0.0f;
        }
    };

    std::vector<Chunk> chunks;

    float resolution = 10.0f;
};

void OpenXRTutorial_Main(GraphicsAPI_Type apiType) {
    DebugOutput debugOutput;  // This redirects std::cerr and std::cout to the IDE's output or Android Studio's logcat.
    XR_LOG("VRPointCloud");

    OpenXR app(apiType);
    app.Run();
}