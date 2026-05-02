#pragma once

// ============================================================================
// 【ShapePool.hpp】形状对象池
// ============================================================================
// 解决每帧创建/销毁 SFML 形状对象导致的性能问题。
//
// 性能问题背景：
// - BattleRenderer::RenderAmbientParticles_ 每帧创建 kMaxAmbientParticles 个 CircleShape
// - WorldRenderer::RenderTeaPlots_ 每帧创建多个 CircleShape/RectangleShape
// - 频繁的 new/delete 导致内存碎片和 GC 压力
//
// 解决方案：
// - 预分配形状对象池
// - Acquire() 获取形状引用 → 使用 → 形状在池内复用
// - 形状属性（位置、大小、颜色）通过引用修改，无需重新创建
//
// 使用示例：
//   ShapePool<sf::CircleShape> pool(100);
//   sf::CircleShape& shape = pool.Acquire();
//   shape.setRadius(5.0f);
//   window.draw(shape);
//   // 无需释放，Acquire 会自动覆盖之前获取的形状
// ============================================================================

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <vector>
#include <algorithm>

namespace CloudSeamanor::engine::rendering {

// ============================================================================
// 【ShapePool】模板形状对象池
// ============================================================================
// 基于引用的简单对象池，避免 unique_ptr 的移动语义复杂性。
//
// 设计原则：
// - Acquire() 返回引用，调用者直接使用
// - 形状在获取时被标记为"已使用"，渲染完成后可被覆盖
// - 无需显式 Release()，简化使用
// - 预分配固定数量的形状，避免运行时分配
// ============================================================================
template <typename ShapeType>
class ShapePool {
public:
    /** @brief 预分配指定数量的形状对象 */
    explicit ShapePool(std::size_t initial_capacity = 64)
        : shapes_(), in_use_count_(0) {
        shapes_.reserve(initial_capacity);
        for (std::size_t i = 0; i < initial_capacity; ++i) {
            shapes_.emplace_back();
        }
    }

    // ==========================================================================
    // 【Acquire】获取形状
    // ==========================================================================
    // 从池中获取一个形状对象。如果池已满，自动扩展。
    //
    // @return 对形状对象的引用，调用者可直接修改属性
    // ==========================================================================
    [[nodiscard]] ShapeType& Acquire() {
        if (shapes_.size() <= in_use_count_) {
            shapes_.emplace_back();
        }
        return shapes_[in_use_count_++];
    }

    // ==========================================================================
    // 【AcquireAll】获取所有形状（用于批量渲染）
    // ==========================================================================
    // 获取 [begin, begin + count) 范围内的形状引用。
    //
    // @param begin 开始索引
    // @param count 要获取的数量
    // @return 对应范围的引用向量
    // ==========================================================================
    [[nodiscard]] std::vector<std::reference_wrapper<ShapeType>> AcquireAll(
        std::size_t begin, std::size_t count
    ) {
        std::vector<std::reference_wrapper<ShapeType>> result;
        result.reserve(count);

        // 扩展池如果需要
        const std::size_t end = begin + count;
        while (shapes_.size() < end) {
            shapes_.emplace_back();
        }

        for (std::size_t i = begin; i < end; ++i) {
            result.emplace_back(shapes_[i]);
        }
        return result;
    }

    // ==========================================================================
    // 【Reset】重置使用计数
    // ==========================================================================
    // 在每帧渲染开始前调用，重置使用计数，使所有形状可被重新获取。
    // ==========================================================================
    void Reset() noexcept {
        in_use_count_ = 0;
    }

    // ==========================================================================
    // 【Preallocate】预分配更多形状
    // ==========================================================================
    // 提前分配指定数量的形状，避免运行时扩展。
    //
    // @param count 要预分配的数量
    // ==========================================================================
    void Preallocate(std::size_t count) {
        const std::size_t target = shapes_.size() + count;
        shapes_.reserve(target);
        while (shapes_.size() < target) {
            shapes_.emplace_back();
        }
    }

    // ==========================================================================
    // 【Clear】清空池
    // ==========================================================================
    // 清空所有形状。
    // ==========================================================================
    void Clear() {
        shapes_.clear();
        in_use_count_ = 0;
    }

    // ==========================================================================
    // 【Stats】统计信息
    // ==========================================================================
    [[nodiscard]] std::size_t Capacity() const noexcept { return shapes_.size(); }
    [[nodiscard]] std::size_t InUseCount() const noexcept { return in_use_count_; }

private:
    std::vector<ShapeType> shapes_;
    std::size_t in_use_count_ = 0;
};

// ============================================================================
// 【CircleShapePool】专门化的圆形对象池
// ============================================================================
using CircleShapePool = ShapePool<sf::CircleShape>;

// ============================================================================
// 【RectShapePool】专门化的矩形对象池
// ============================================================================
using RectShapePool = ShapePool<sf::RectangleShape>;

// ============================================================================
// 【GlobalShapePools】全局形状池（单例）
// ============================================================================
// 提供全局共享的形状池，避免每个渲染器独立创建池。
// ============================================================================
class GlobalShapePools {
public:
    static GlobalShapePools& Instance() {
        static GlobalShapePools instance;
        return instance;
    }

    // 圆形池：用于粒子、心形特效、光晕等
    CircleShapePool& Circles() { return circle_pool_; }

    // 矩形池：用于地块、边框、进度条等
    RectShapePool& Rects() { return rect_pool_; }

    /** @brief 预分配所有池 */
    void PreallocateAll() {
        circle_pool_.Preallocate(256);
        rect_pool_.Preallocate(256);
    }

private:
    GlobalShapePools() {
        PreallocateAll();
    }

    CircleShapePool circle_pool_{64};
    RectShapePool rect_pool_{64};
};

}  // namespace CloudSeamanor::engine::rendering
