#include "CloudSeamanor/engine/systems/PetSystem.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/engine/systems/SpiritBeastSystem.hpp"

#include <cmath>
#include <string>

namespace CloudSeamanor::engine {

void PetSystem::Update(GameWorldState& world_state, float delta_seconds) const {
    if (!world_state.GetPetAdopted()) {
        return;
    }

    // P14-BLOCK-002/P13-GAP-001 修复：统一灵兽位置更新入口
    // 如果 SpiritBeastSystem 正在管理灵兽跟随，PetSystem 不再更新位置
    // 这样可以避免 PetSystem 和 SpiritBeastSystem 竞争位置更新
    // 注意：这个检查依赖于 SpiritBeastSystem 的单例或其他协调机制
    // 当前实现中，PetSystem 只负责装饰性动画，不做跟随逻辑

    // Bird 类型的装饰性动画（浮动效果）
    // 这个效果由世界渲染系统处理，PetSystem 只负责更新动画状态
    // 不再直接修改 world_state 中的灵兽位置，避免与 SpiritBeastSystem 冲突

    // 如果需要装饰性效果，应该通过事件/通知机制让渲染层处理
    // 或者添加一个独立的 decoration_offset 字段

    // 当前：PetSystem 不做任何跟随或位置更新
    // 跟随逻辑完全由 SpiritBeastSystem 统一管理
}

} // namespace CloudSeamanor::engine
