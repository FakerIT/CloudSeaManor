#pragma once

// ============================================================================
// 【AllDefine.hpp】遗留兼容壳（Deprecated）
// ============================================================================
// 说明：
// - 该头文件已不再作为项目内部“万能聚合头”使用。
// - 新代码必须按需 include 具体头文件，禁止新增对本文件的直接依赖。
// - 若历史代码短期仍需兼容，请在编译定义中显式开启：
//     CLOUDSEAMANOR_ENABLE_LEGACY_ALLDEFINE=1
//
// 设计目标：
// 1) 默认阻止误用，避免回归“传染式 include”。
// 2) 提供受控兼容通道，便于渐进迁移。
// ============================================================================

#if !defined(CLOUDSEAMANOR_ENABLE_LEGACY_ALLDEFINE) || (CLOUDSEAMANOR_ENABLE_LEGACY_ALLDEFINE != 1)
#error "AllDefine.hpp is deprecated. Include specific headers instead. If you must keep legacy compatibility temporarily, define CLOUDSEAMANOR_ENABLE_LEGACY_ALLDEFINE=1."
#endif

#if defined(_MSC_VER)
#pragma message("warning: CloudSeamanor/AllDefine.hpp is deprecated and will be removed. Please include concrete headers.")
#endif

// 兼容模式下，统一转发到对外聚合主头。
#include "CloudSeamanor/CloudSeaManor.hpp"
