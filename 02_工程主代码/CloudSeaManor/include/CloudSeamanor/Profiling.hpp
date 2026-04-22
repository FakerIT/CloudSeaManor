#pragma once

// ============================================================================
// 【Profiling】轻量性能埋点（Tracy 可选）
// ============================================================================
// - 默认不引入任何外部依赖，不影响构建。
// - 若工程引入 Tracy，可在编译定义 TRACY_ENABLE，并确保可 include <tracy/Tracy.hpp>。
//   则以下宏会自动映射到 Tracy 的 ZoneScoped/FrameMark。
// ============================================================================

#if defined(TRACY_ENABLE)
    #if defined(__has_include)
        #if __has_include(<tracy/Tracy.hpp>)
            #include <tracy/Tracy.hpp>
            #define CSM_ZONE_SCOPED ZoneScoped
            #define CSM_FRAME_MARK FrameMark
        #else
            #define CSM_ZONE_SCOPED ((void)0)
            #define CSM_FRAME_MARK ((void)0)
        #endif
    #else
        #define CSM_ZONE_SCOPED ((void)0)
        #define CSM_FRAME_MARK ((void)0)
    #endif
#else
    #define CSM_ZONE_SCOPED ((void)0)
    #define CSM_FRAME_MARK ((void)0)
#endif

