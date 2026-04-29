#include "CloudSeamanor/app/GameAppFarming.hpp"

#include "CloudSeamanor/domain/CropData.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

namespace {
std::string g_crop_table_data_path = "assets/data/CropTable.csv";

bool IsTeaCropId_(const std::string& crop_id) {
    return crop_id.find("tea") != std::string::npos;
}

// ============================================================================
// 【InitCropTableFromFile】惰性加载作物表
// ============================================================================
const CloudSeamanor::domain::CropTable& CropTableInstance() {
    static CloudSeamanor::domain::CropTable table;
    static bool loaded = false;
    static std::string loaded_path;
    if (!loaded) {
        table.LoadFromFile(g_crop_table_data_path);
        loaded_path = g_crop_table_data_path;
        loaded = true;
    } else if (loaded_path != g_crop_table_data_path) {
        table = CloudSeamanor::domain::CropTable{};
        table.LoadFromFile(g_crop_table_data_path);
        loaded_path = g_crop_table_data_path;
    }
    return table;
}

// ============================================================================
// 【BuildPlotFromCropDef】根据作物定义构建地块
// ============================================================================
TeaPlot BuildPlotFromCropDef(const CloudSeamanor::domain::CropDefinition& def,
                              const sf::Vector2f& position,
                              bool highlighted) {
    TeaPlot plot;
    plot.crop_id = def.id;
    plot.layer = IsTeaCropId_(def.id) ? TeaPlotLayer::TeaGardenExclusive : TeaPlotLayer::NormalFarm;
    plot.crop_name = def.name;
    plot.seed_item_id = def.seed_item_id;
    plot.harvest_item_id = def.harvest_item_id;
    plot.harvest_amount = def.base_harvest;
    plot.growth_time = def.growth_time;
    plot.growth_stages = def.stages;
    plot.size = {52.0f, 52.0f};
    plot.position = CloudSeamanor::adapter::ToDomain(position);
    RefreshTeaPlotVisual(plot, highlighted);
    return plot;
}

void ApplyObstacleVisual(TeaPlot& plot, bool highlighted) {
    switch (plot.obstacle_type) {
    case PlotObstacleType::Stone:
        plot.fill_rgba = PackRgba(86, 90, 96);
        break;
    case PlotObstacleType::Stump:
        plot.fill_rgba = PackRgba(92, 66, 38);
        break;
    case PlotObstacleType::Weed:
        plot.fill_rgba = PackRgba(52, 90, 52);
        break;
    case PlotObstacleType::None:
    default:
        plot.fill_rgba = PackRgba(58, 62, 66);
        break;
    }
    plot.outline_rgba = highlighted ? PackRgba(255, 255, 255) : PackRgba(34, 34, 34);
    plot.outline_thickness = highlighted ? 4.0f : 2.0f;
}

void ApplyReadyPlotVisual(TeaPlot& plot, bool highlighted) {
    switch (plot.quality) {
    case domain::CropQuality::Spirit:
        plot.fill_rgba = PackRgba(210, 180, 255);
        break;
    case domain::CropQuality::Rare:
        plot.fill_rgba = PackRgba(255, 215, 80);
        break;
    case domain::CropQuality::Fine:
        plot.fill_rgba = PackRgba(160, 230, 120);
        break;
    default:
        plot.fill_rgba = PackRgba(132, 214, 126);
        break;
    }
    plot.outline_rgba = highlighted ? PackRgba(255, 255, 255) : PackRgba(60, 110, 58);
}

TeaPlot BuildLockedPlotFromCropDef(const CloudSeamanor::domain::CropDefinition& def,
                                   const sf::Vector2f& position) {
    auto plot = BuildPlotFromCropDef(def, position, false);
    plot.cleared = false;
    plot.obstacle_type = PlotObstacleType::Stone;
    plot.obstacle_hits_left = 3;
    plot.tilled = false;
    plot.seeded = false;
    plot.watered = false;
    plot.ready = false;
    RefreshTeaPlotVisual(plot, false);
    return plot;
}

} // namespace

void SetCropTableDataPath(const std::string& crop_table_path) {
    if (!crop_table_path.empty()) {
        g_crop_table_data_path = crop_table_path;
    }
}

// ============================================================================
// 【RefreshTeaPlotVisual】刷新地块视觉状态
// ============================================================================
void RefreshTeaPlotVisual(TeaPlot& plot, bool highlighted) {
    if (!plot.cleared) {
        ApplyObstacleVisual(plot, highlighted);
        return;
    }
    if (!plot.tilled) {
        plot.fill_rgba = PackRgba(78, 118, 74);
        plot.outline_rgba = highlighted ? PackRgba(255, 255, 255) : PackRgba(58, 88, 50);
    } else if (!plot.seeded) {
        plot.fill_rgba = PackRgba(124, 88, 56);
        plot.outline_rgba = highlighted ? PackRgba(255, 255, 255) : PackRgba(88, 60, 38);
    } else if (!plot.watered) {
        plot.fill_rgba = PackRgba(146, 104, 64);
        plot.outline_rgba = highlighted ? PackRgba(255, 255, 255) : PackRgba(84, 62, 44);
    } else if (plot.ready) {
        ApplyReadyPlotVisual(plot, highlighted);
    } else {
        // 生长中：根据阶段调整绿色深浅
        const float stage_ratio =
            domain::CropTable::StageGrowthThreshold(plot.stage, plot.growth_stages);
        const std::uint8_t green = static_cast<std::uint8_t>(82 + stage_ratio * 116);
        const std::uint8_t blue_hint = plot.watered ? 16 : 0;
        plot.fill_rgba = PackRgba(92, green, static_cast<std::uint8_t>(82 + blue_hint));
        plot.outline_rgba = highlighted ? PackRgba(255, 255, 255) : PackRgba(56, 96, 54);
    }
    plot.outline_thickness = highlighted ? 4.0f : 2.0f;
}

// ============================================================================
// 【BuildTeaPlots】构建默认地块列表（数据驱动版本）
// ============================================================================
void BuildTeaPlots(std::vector<TeaPlot>& tea_plots) {
    tea_plots.clear();
    constexpr float start_x = 392.0f;
    constexpr float start_y = 360.0f;
    constexpr float gap = 64.0f;

    const auto& table = CropTableInstance();

    // 前 2 块：云雾茶
    for (int i = 0; i < 2; ++i) {
        if (const auto* def = table.Get("tea_leaf")) {
            tea_plots.push_back(BuildPlotFromCropDef(
                *def,
                sf::Vector2f(start_x + gap * static_cast<float>(i), start_y),
                false));
        }
    }
    // 第 3 块：萝卜
    if (const auto* def = table.Get("turnip")) {
        tea_plots.push_back(BuildPlotFromCropDef(
            *def,
            sf::Vector2f(start_x + gap * 2.0f, start_y),
            false));
    }

    // S-7：额外 1 块待开垦地（需要资源开垦后才能耕种）
    if (const auto* def = table.Get("turnip")) {
        tea_plots.push_back(BuildLockedPlotFromCropDef(
            *def,
            sf::Vector2f(start_x + gap * 3.0f, start_y)));
    }
}

// ============================================================================
// 【UpdateCropGrowth】更新作物生长
// ============================================================================
bool UpdateCropGrowth(
    std::vector<TeaPlot>& plots,
    float delta_seconds,
    float cloud_multiplier,
    float spirit_buff,
    float beast_share,
    const std::function<void(TeaPlot&, bool)>& refresh_visual,
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info
) {
    // Unified growth update path now lives in GameRuntime/CropGrowthSystem.
    // Keep this API as a compatibility shim for legacy call sites.
    (void)delta_seconds;
    (void)cloud_multiplier;
    (void)spirit_buff;
    (void)beast_share;
    (void)refresh_visual;
    (void)push_hint;
    (void)log_info;
    return std::any_of(plots.begin(), plots.end(), [](const TeaPlot& plot) {
        return plot.ready;
    });
}

// ============================================================================
// 【HandlePlotInteraction】处理地块交互
// ============================================================================
bool HandlePlotInteraction(
    TeaPlot& plot,
    CloudSeamanor::domain::Inventory& inventory,
    CloudSeamanor::domain::SkillSystem& skills,
    CloudSeamanor::domain::CloudState cloud_state,
    float cloud_density,
    bool beast_interacted,
    const std::function<void(TeaPlot&, bool)>& refresh_visual,
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info
) {
    if (!plot.tilled) {
        plot.tilled = true;
        push_hint(plot.crop_name + " 地块已翻土。下一步：播种。", 2.4f);
        log_info(plot.crop_name + " 地块已翻土。");
        return true;
    }

    if (!plot.seeded) {
        if (inventory.RemoveItem(plot.seed_item_id, 1)) {
            // 播种时根据云海状态决定品质
            plot.quality = domain::CropTable::CalculateQuality(cloud_state, false);
            plot.seeded = true;
            plot.growth = 0.0f;
            plot.stage = 1;
            refresh_visual(plot, false);
            push_hint(plot.crop_name + " 已播种。记得浇水后作物才会开始生长。", 2.6f);
            log_info(plot.crop_name + " 已播种（品质：" +
                     domain::CropTable::QualityToText(plot.quality) + "）。");
            return true;
        } else {
            push_hint("缺少种子：" + ItemDisplayName(plot.seed_item_id) + "。", 2.6f);
            log_info("缺少种子：" + plot.seed_item_id);
            return false;
        }
    }

    if (!plot.watered) {
        plot.watered = true;
        refresh_visual(plot, false);
        push_hint(plot.crop_name + " 已浇水。好好享受今天的云海吧！", 3.0f);
        log_info(plot.crop_name + " 地块已浇水。");
        return true;
    }

    if (plot.ready) {
        // 收获时根据当前云海状态计算最终品质
        plot.quality = domain::CropTable::CalculateQuality(cloud_state, false);

        const float tea_buff = 1.0f + skills.GetBonus(CloudSeamanor::domain::SkillType::SpiritFarm) * 0.1f;
        const float beast_share_factor = beast_interacted ? 1.2f : 1.0f;
        if (skills.AddExp(CloudSeamanor::domain::SkillType::SpiritFarm,
                          20.0f, cloud_density, tea_buff, beast_share_factor)) {
            const int new_level = skills.GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
            push_hint("灵农技能提升至 Lv." + std::to_string(new_level) + "！加成效果增强。", 3.2f);
            log_info("灵农技能升级至 Lv." + std::to_string(new_level) + "！");
        }

        // 品质影响实际收获数量
        const float quality_mult = domain::CropTable::QualityHarvestMultiplier(plot.quality);
        const int actual_amount = std::max(1, static_cast<int>(
            static_cast<float>(plot.harvest_amount) * quality_mult));

        const std::string quality_prefix =
            domain::CropTable::QualityToPrefixText(plot.quality);

        inventory.AddItem(plot.harvest_item_id, actual_amount);
        plot.seeded = false;
        plot.watered = false;
        plot.ready = false;
        plot.growth = 0.0f;
        plot.stage = 0;
        plot.quality = domain::CropQuality::Normal;

        push_hint("已收获 " + quality_prefix + ItemDisplayName(plot.harvest_item_id) +
                  " x" + std::to_string(actual_amount) + "。", 2.8f);
        log_info(plot.crop_name + " 已收获（品质：" +
                 domain::CropTable::QualityToText(plot.quality) +
                 "，数量：" + std::to_string(actual_amount) + "）。");
        return true;
    }

    push_hint(plot.crop_name + " 还在生长中。" + PlotStatusText(plot) + "。", 2.2f);
    log_info(plot.crop_name + " 仍在生长中。");
    return true;
}

// ============================================================================
// 【PlotStatusText】获取地块状态文本
// ============================================================================
std::string PlotStatusText(const TeaPlot& plot) {
    if (!plot.tilled) return "未开垦";
    if (!plot.seeded) return "等待播种";
    if (!plot.watered) return "需要浇水";
    if (plot.ready) {
        const std::string quality_prefix =
            domain::CropTable::QualityToText(plot.quality);
        return "可收获（" + std::string(quality_prefix) + "）";
    }
    const float progress_pct = (plot.growth_time > 0.0f)
        ? std::min(100.0f, (plot.growth / plot.growth_time) * 100.0f)
        : 0.0f;
    return "生长中 " + std::to_string(plot.stage) + "/" +
           std::to_string(plot.growth_stages) +
           "（" + std::to_string(progress_pct) + "%）";
}

// ============================================================================
// 【CountReadyPlots】统计可收获地块数量
// ============================================================================
int CountReadyPlots(const std::vector<TeaPlot>& plots) {
    int count = 0;
    for (const auto& plot : plots) {
        if (plot.ready) ++count;
    }
    return count;
}

// ============================================================================
// 【CountDryPlots】统计缺水地块数量
// ============================================================================
int CountDryPlots(const std::vector<TeaPlot>& plots) {
    int count = 0;
    for (const auto& plot : plots) {
        if (!plot.ready && plot.seeded && !plot.watered) ++count;
    }
    return count;
}

// ============================================================================
// 【CountGrowingPlots】统计生长中地块数量
// ============================================================================
int CountGrowingPlots(const std::vector<TeaPlot>& plots) {
    int count = 0;
    for (const auto& plot : plots) {
        if (plot.seeded && plot.watered && !plot.ready) ++count;
    }
    return count;
}

} // namespace CloudSeamanor::engine
