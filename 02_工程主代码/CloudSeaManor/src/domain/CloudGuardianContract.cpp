#include "CloudSeamanor/domain/CloudGuardianContract.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::domain {

namespace {

// ============================================================================
// 【InitializeVolumes_】初始化6卷契约数据
// ============================================================================
std::vector<ContractVolume> CreateDefaultVolumes() {
    std::vector<ContractVolume> vols;

    // ========================================================================
    // 卷1：茶园苏醒契（初始解锁）
    // ========================================================================
    {
        ContractVolume v;
        v.volume_id = 1;
        v.name = "茶园苏醒契";
        v.theme = "农业与灵茶";
        v.description = "照料茶园，唤醒山庄的生机。收集灵茶与作物，证明你的勤劳。";
        v.permanent_bonus_description = "茶园生长速度+25%，自动采摘机解锁";
        v.unlocked = true;  // 第一卷初始解锁
        v.items = {
            {"pact_1_1", "春之芽", "收集初雾绿茶×10和翠竹笋×15", "TeaLeaf", 10},
            {"pact_1_2", "夏之云", "收集夏云白茶×12、蜜桃×20、蓝莓×10", "TeaLeaf", 12},
            {"pact_1_3", "秋之霜", "收集秋霜乌龙×15和任意灵茶×30", "TeaLeaf", 15},
            {"pact_1_4", "冬之雪", "收集冬雪银针×10和雪莲根×20", "TeaLeaf", 10},
        };
        vols.push_back(std::move(v));
    }

    // ========================================================================
    // 卷2：山庄修缮契
    // ========================================================================
    {
        ContractVolume v;
        v.volume_id = 2;
        v.name = "山庄修缮契";
        v.theme = "建筑与装修";
        v.description = "修缮山庄的每一处破损，让这里成为真正的家。";
        v.permanent_bonus_description = "山庄灵气每日+30，新游客类型开放";
        v.unlocked = false;
        v.items = {
            {"pact_2_1", "主屋新生", "使用木材×10和萝卜×6修缮主屋", "Wood", 10},
            {"pact_2_2", "茶园扩建", "建造或升级茶园设施", "Wood", 15},
            {"pact_2_3", "工坊升级", "升级制茶机或其他工坊设备", "Wood", 20},
        };
        vols.push_back(std::move(v));
    }

    // ========================================================================
    // 卷3：灵兽共生契
    // ========================================================================
    {
        ContractVolume v;
        v.volume_id = 3;
        v.name = "灵兽共生契";
        v.theme = "灵兽系统";
        v.description = "与灵兽建立深厚的羁绊，共同守护云海山庄。";
        v.permanent_bonus_description = "灵兽效率+50%，自由繁殖解锁";
        v.unlocked = false;
        v.items = {
            {"pact_3_1", "初次结缘", "与灵兽建立第一次羁绊", "TeaLeaf", 5},
            {"pact_3_2", "每日相伴", "连续7天与灵兽互动", "TeaLeaf", 7},
            {"pact_3_3", "灵兽图鉴", "收集3种不同的灵兽互动", "TeaLeaf", 10},
        };
        vols.push_back(std::move(v));
    }

    // ========================================================================
    // 卷4：云海羁绊契
    // ========================================================================
    {
        ContractVolume v;
        v.volume_id = 4;
        v.name = "云海羁绊契";
        v.theme = "社交与角色";
        v.description = "与村民建立深厚的友谊，让山庄充满温情。";
        v.permanent_bonus_description = "全村每日好感+15，婚后配偶自动帮忙";
        v.unlocked = false;
        v.items = {
            {"pact_4_1", "初次相识", "与任意NPC完成第一次对话", "TeaLeaf", 3},
            {"pact_4_2", "友谊之礼", "送出10份茶包作为礼物", "TeaPack", 10},
            {"pact_4_3", "心意相通", "将任意角色好感度提升至10心", "TeaPack", 5},
        };
        vols.push_back(std::move(v));
    }

    // ========================================================================
    // 卷5：灵界探索契
    // ========================================================================
    {
        ContractVolume v;
        v.volume_id = 5;
        v.name = "灵界探索契";
        v.theme = "探索与采集";
        v.description = "探索灵界的奥秘，收集珍稀材料。";
        v.permanent_bonus_description = "灵界掉落翻倍，隐藏区域解锁";
        v.unlocked = false;
        v.items = {
            {"pact_5_1", "初次探索", "进入灵界进行探索", "TeaLeaf", 5},
            {"pact_5_2", "材料收集", "收集稀有灵界材料×15", "TeaLeaf", 15},
            {"pact_5_3", "界桥碎片", "收集界桥碎片×3", "TeaLeaf", 10},
        };
        vols.push_back(std::move(v));
    }

    // ========================================================================
    // 卷6：太初平衡契
    // ========================================================================
    {
        ContractVolume v;
        v.volume_id = 6;
        v.name = "太初平衡契";
        v.theme = "主线与终局";
        v.description = "完成前5卷的契约，达成云海守护者的最终使命。";
        v.permanent_bonus_description = "真结局永久解锁，云海最低锁定薄雾，圣品概率+30%";
        v.unlocked = false;
        v.items = {
            {"pact_6_1", "集齐前卷", "完成卷1-5的所有云海之约", "TeaLeaf", 50},
            {"pact_6_2", "太初元茶", "收集太初元茶×1", "TeaLeaf", 30},
            {"pact_6_3", "界桥果", "收集界桥果×1", "TeaLeaf", 20},
        };
        vols.push_back(std::move(v));
    }

    return vols;
}

}  // namespace

// ============================================================================
// 【Initialize】初始化契约数据
// ============================================================================
void CloudGuardianContract::Initialize() {
    volumes_ = CreateDefaultVolumes();
}

// ============================================================================
// 【GetVolume】获取指定卷
// ============================================================================
const ContractVolume* CloudGuardianContract::GetVolume(int volume_id) const {
    for (const auto& v : volumes_) {
        if (v.volume_id == volume_id) {
            return &v;
        }
    }
    return nullptr;
}

// ============================================================================
// 【GetTrackingVolume】获取当前追踪卷
// ============================================================================
const ContractVolume* CloudGuardianContract::GetTrackingVolume() const {
    return GetVolume(tracking_volume_id_);
}

// ============================================================================
// 【SetTrackingVolume】设置追踪卷
// ============================================================================
void CloudGuardianContract::SetTrackingVolume(int volume_id) {
    if (IsVolumeUnlocked(volume_id)) {
        tracking_volume_id_ = volume_id;
    }
}

// ============================================================================
// 【DeliverItem】交付物品（简化版）
// ============================================================================
bool CloudGuardianContract::DeliverItem(const std::string& item_id, int count) {
    (void)count;  // 简化版忽略数量
    bool any_completed = false;

    for (auto& vol : volumes_) {
        if (!vol.unlocked) continue;

        for (auto& item : vol.items) {
            if (item.completed) continue;

            if (item.required_item == item_id) {
                item.completed = true;
                any_completed = true;
            }
        }
    }

    return any_completed;
}

// ============================================================================
// 【CheckVolumeUnlocks】检查卷解锁
// ============================================================================
void CloudGuardianContract::CheckVolumeUnlocks() {
    // 检查前序卷是否完成，解锁后续卷
    for (auto& vol : volumes_) {
        if (vol.unlocked || vol.completed) continue;

        bool predecessor_completed = true;
        for (const auto& v : volumes_) {
            if (v.volume_id < vol.volume_id && !v.completed) {
                predecessor_completed = false;
                break;
            }
        }

        if (predecessor_completed && vol.volume_id > 1) {
            vol.unlocked = true;
        }
    }

    // 检查已解锁卷是否全部完成
    for (auto& vol : volumes_) {
        if (!vol.unlocked || vol.completed) continue;

        bool all_items_completed = true;
        for (const auto& item : vol.items) {
            if (!item.completed) {
                all_items_completed = false;
                break;
            }
        }

        if (all_items_completed) {
            vol.completed = true;
        }
    }
}

// ============================================================================
// 【ProgressText】总体进度文本
// ============================================================================
std::string CloudGuardianContract::ProgressText() const {
    const int completed = CompletedPactCount();
    const int total = TotalPactCount();
    return "云海守护者契约：已完成 " + std::to_string(completed) + "/" +
           std::to_string(total) + " 个云海之约，卷 " +
           std::to_string(CompletedVolumeCount()) + "/6 已完成。";
}

// ============================================================================
// 【VolumeProgressText】指定卷进度文本
// ============================================================================
std::string CloudGuardianContract::VolumeProgressText(int volume_id) const {
    const auto* vol = GetVolume(volume_id);
    if (!vol) return "未知卷";

    int completed = vol->CompletedCount();

    std::string status = vol->completed ? "【已完成】" :
                        (vol->unlocked ? "【进行中】" : "【未解锁】");
    return status + vol->name + "：" + std::to_string(completed) + "/" +
           std::to_string(vol->TotalCount()) + " 已完成";
}

// ============================================================================
// 【TodayRecommendation】今日推荐
// ============================================================================
std::string CloudGuardianContract::TodayRecommendation() const {
    const auto* tracking = GetTrackingVolume();
    if (!tracking) return "当前无追踪契约。";

    for (const auto& item : tracking->items) {
        if (!item.completed) {
            return "今日推荐推进【" + tracking->name + "】：" + item.name +
                   "，需要收集 " + item.required_item + " ×" +
                   std::to_string(item.required_count);
        }
    }
    return "【" + tracking->name + "】当前项已全部完成，可切换追踪卷。";
}

AuraStage CloudGuardianContract::CurrentAuraStage() const noexcept {
    const int total = TotalPactCount();
    if (total <= 0) {
        return AuraStage::Barren;
    }
    const float ratio = static_cast<float>(CompletedPactCount()) / static_cast<float>(total);
    const float percent = ratio * 100.0f;
    if (percent < 20.0f) return AuraStage::Barren;
    if (percent < 50.0f) return AuraStage::Awakened;
    if (percent < 80.0f) return AuraStage::Flourish;
    if (percent < 95.0f) return AuraStage::Prosper;
    return AuraStage::Primordial;
}

std::string CloudGuardianContract::CurrentAuraStageText() const {
    switch (CurrentAuraStage()) {
    case AuraStage::Barren: return "荒芜";
    case AuraStage::Awakened: return "苏醒";
    case AuraStage::Flourish: return "兴盛";
    case AuraStage::Prosper: return "繁华";
    case AuraStage::Primordial: return "太初";
    }
    return "荒芜";
}

// ============================================================================
// 【CompletedVolumeCount】已完成卷数
// ============================================================================
int CloudGuardianContract::CompletedVolumeCount() const noexcept {
    int count = 0;
    for (const auto& v : volumes_) {
        if (v.completed) ++count;
    }
    return count;
}

// ============================================================================
// 【CompletedPactCount】已完成云海之约数
// ============================================================================
int CloudGuardianContract::CompletedPactCount() const noexcept {
    int count = 0;
    for (const auto& v : volumes_) {
        count += v.CompletedCount();
    }
    return count;
}

// ============================================================================
// 【TotalPactCount】总云海之约数
// ============================================================================
int CloudGuardianContract::TotalPactCount() const noexcept {
    int count = 0;
    for (const auto& v : volumes_) {
        count += v.TotalCount();
    }
    return count;
}

// ============================================================================
// 【IsVolumeUnlocked】卷是否已解锁
// ============================================================================
bool CloudGuardianContract::IsVolumeUnlocked(int volume_id) const {
    const auto* vol = GetVolume(volume_id);
    return vol && vol->unlocked;
}

// ============================================================================
// 【SaveState】保存契约状态
// ============================================================================
std::string CloudGuardianContract::SaveState() const {
    std::ostringstream oss;

    for (const auto& vol : volumes_) {
        oss << "v" << vol.volume_id << ":"
             << (vol.unlocked ? "1" : "0") << ","
             << (vol.completed ? "1" : "0") << "|";

        for (const auto& item : vol.items) {
            oss << item.id << ":" << (item.completed ? "1" : "0") << ",";
        }
        oss << ";";
    }

    return oss.str();
}

// ============================================================================
// 【LoadState】加载契约状态
// ============================================================================
void CloudGuardianContract::LoadState(const std::string& state) {
    if (state.empty()) return;

    Initialize();
    std::size_t start = 0;
    while (start < state.size()) {
        const auto seg_end = state.find(';', start);
        const auto seg = state.substr(start, seg_end == std::string::npos ? std::string::npos : seg_end - start);
        start = (seg_end == std::string::npos) ? state.size() : seg_end + 1;
        if (seg.empty()) continue;

        const auto bar = seg.find('|');
        if (bar == std::string::npos) continue;
        const auto head = seg.substr(0, bar);
        const auto items = seg.substr(bar + 1);

        if (head.size() < 3 || head[0] != 'v') continue;
        const auto c1 = head.find(':');
        const auto c2 = head.find(',', c1 == std::string::npos ? 0 : c1 + 1);
        if (c1 == std::string::npos || c2 == std::string::npos) continue;

        const int vid = std::stoi(head.substr(1, c1 - 1));
        const bool unlocked = head.substr(c1 + 1, c2 - c1 - 1) == "1";
        const bool completed = head.substr(c2 + 1) == "1";
        for (auto& vol : volumes_) {
            if (vol.volume_id != vid) continue;
            vol.unlocked = unlocked;
            vol.completed = completed;

            std::size_t p = 0;
            while (p < items.size()) {
                const auto comma = items.find(',', p);
                const auto token = items.substr(p, comma == std::string::npos ? std::string::npos : comma - p);
                p = (comma == std::string::npos) ? items.size() : comma + 1;
                if (token.empty()) continue;
                const auto colon = token.find(':');
                if (colon == std::string::npos) continue;
                const std::string item_id = token.substr(0, colon);
                const bool done = token.substr(colon + 1) == "1";
                for (auto& it : vol.items) {
                    if (it.id == item_id) {
                        it.completed = done;
                        break;
                    }
                }
            }
            break;
        }
    }
}

// ============================================================================
// 【Reset】重置契约进度
// ============================================================================
void CloudGuardianContract::Reset() {
    Initialize();
}

// ============================================================================
// 【GetGlobalContract】获取全局单例
// ============================================================================
CloudGuardianContract& GetGlobalContract() {
    static CloudGuardianContract instance;
    return instance;
}

}  // namespace CloudSeamanor::domain
