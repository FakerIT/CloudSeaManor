#pragma once

#include <string>
#include <vector>

namespace CloudSeamanor::prompt_templates {

struct PromptTemplate {
    std::string category;
    std::string template_prompt;
    std::string negative_prompt;
    std::string description_zh;
};

class PromptTemplateLibrary {
public:
    static const std::vector<PromptTemplate>& GetCharacterPortraitTemplates() {
        static std::vector<PromptTemplate> templates = {
            {
                "武侠仙侠风格",
                "beautiful anime character portrait, young woman, long flowing black hair with ornate silver hairpin, "
                "elegant xianxia style hanfu with cloud embroidery patterns, gentle serene expression, "
                "holding a jade ornament, soft ethereal lighting, detailed face, beautiful eyes, "
                "high quality illustration, upper body shot, traditional chinese aesthetic",
                "low quality, blurry, bad anatomy, deformed, ugly, bad hands, extra fingers, "
                "extra limbs, poorly drawn face, mutation, mutated",
                "武侠仙侠风格女性角色立绘，精致汉服"
            },
            {
                "活泼可爱风格",
                "anime character portrait, cute young girl, short bob hair with red ribbon, "
                "colorful traditional chinese dress with flower patterns, cheerful smile, sparkling eyes, "
                "holding a basket of flowers, bright and warm lighting, joyful atmosphere, "
                "high quality illustration, full body shot, kawaii style",
                "low quality, blurry, bad anatomy, deformed, ugly, bad hands, extra fingers, "
                "dark, somber, sad expression",
                "活泼可爱风格角色立绘，彩色传统服饰"
            },
            {
                "冷艳高傲风格",
                "anime character portrait, beautiful woman with long silver hair, intricate traditional "
                "Chinese hairstyle with pearl ornaments, dark elegant robe with phoenix embroidery, "
                "cold aloof expression, piercing gaze, holding a folding fan, dramatic lighting, "
                "high quality illustration, upper body shot, mature atmosphere",
                "low quality, blurry, bad anatomy, deformed, ugly, bad hands, extra fingers, "
                "cute, childish, happy expression",
                "冷艳高傲风格女性角色立绘"
            },
            {
                "男性剑客风格",
                "anime character portrait, handsome young man with long dark hair tied in high ponytail, "
                "martial arts warrior costume with dragon embroidery, confident smirk, "
                "holding a sword, dynamic pose, dramatic wind effects, strong lighting contrast, "
                "high quality illustration, full body shot, xianxia warrior style",
                "low quality, blurry, bad anatomy, deformed, ugly, bad hands, extra fingers, "
                "extra limbs, feminine features, weak posture",
                "男性剑客角色立绘，武侠风格"
            },
            {
                "儿童角色风格",
                "anime character portrait, adorable young child around 8-10 years old, cute short hair, "
                "simple traditional chinese clothing, innocent curious expression, big sparkling eyes, "
                "holding a small spirit beast plush toy, warm soft lighting, adorable kawaii style, "
                "high quality illustration, upper body shot",
                "low quality, blurry, bad anatomy, deformed, ugly, bad hands, extra fingers, "
                "adult, mature, inappropriate clothing",
                "可爱儿童角色立绘"
            }
        };
        return templates;
    }

    static const std::vector<PromptTemplate>& GetGameSceneTemplates() {
        static std::vector<PromptTemplate> templates = {
            {
                "茶园场景",
                "beautiful vertical scrolling game background scene, chinese xianxia style tea plantation, "
                "terraced tea fields stretching across mountainside, early morning with soft mist, "
                "warm golden sunlight filtering through clouds, traditional chinese tea house in distance, "
                "lush green tea bushes, cherry blossoms falling, peaceful tranquil atmosphere, "
                "high quality game art, detailed environment illustration, soft atmospheric lighting",
                "low quality, blurry, watermark, text, logo, signature, deformed buildings, "
                "modern buildings, cars, people, crowded",
                "云海茶园游戏场景，清晨薄雾"
            },
            {
                "主屋大厅场景",
                "beautiful vertical scrolling game background scene, traditional chinese mansion interior hall, "
                "elegant wooden architecture with carved beams, warm lantern lighting, "
                "antique furniture including tea table and cushions, decorative scrolls on walls, "
                "view through open doors to garden outside, cozy warm atmosphere, "
                "high quality game art, detailed interior illustration, warm color palette",
                "low quality, blurry, watermark, text, logo, signature, modern furniture, "
                "cluttered, dirty, broken furniture",
                "云海山庄主屋大厅场景"
            },
            {
                "温泉场景",
                "beautiful vertical scrolling game background scene, mystical hot spring in xianxia world, "
                "rocky pool with steaming water, ethereal mist rising, traditional stone pavilion nearby, "
                "night sky with stars and full moon, bamboo forest surroundings, magical atmosphere, "
                "high quality game art, detailed environment illustration, romantic mood",
                "low quality, blurry, watermark, text, logo, signature, modern elements, "
                "polluted water, garbage, ugly",
                "仙气缭绕的温泉场景，夜晚星空"
            },
            {
                "灵兽园场景",
                "beautiful vertical scrolling game background scene, magical spirit beast garden, "
                "lush green meadow with exotic flowers, spirit beasts wandering peacefully, "
                "traditional chinese fence enclosure, ancient trees providing shade, "
                "butterflies and fireflies around, sunny day with clouds, vibrant colors, "
                "high quality game art, detailed environment illustration, lively atmosphere",
                "low quality, blurry, watermark, text, logo, signature, deformed animals, "
                "dead trees, garbage, ugly",
                "灵兽园场景，可爱的仙兽"
            },
            {
                "观云台日落场景",
                "beautiful vertical scrolling game background scene, mountain observation platform at sunset, "
                "vast sea of clouds below, golden orange sky with pink clouds, mountain peaks piercing through, "
                "traditional chinese pavilion, dramatic lighting, wind effects, "
                "birds flying in formation, breathtaking majestic scenery, "
                "high quality game art, detailed environment illustration, epic mood",
                "low quality, blurry, watermark, text, logo, signature, modern buildings, "
                "ugly clouds, polluted sky, garbage",
                "观云台日落场景，云海翻涌"
            },
            {
                "工坊场景",
                "beautiful vertical scrolling game background scene, traditional chinese workshop interior, "
                "crafting stations with tools, warm firelight from forge, "
                "shelves with materials and finished crafts, wooden workbenches, "
                "workshop items scattered around, cozy working atmosphere, "
                "high quality game art, detailed interior illustration, warm color palette",
                "low quality, blurry, watermark, text, logo, signature, modern equipment, "
                "cluttered, dirty, broken tools",
                "工坊场景，温馨的工作氛围"
            },
            {
                "客栈场景",
                "beautiful vertical scrolling game background scene, traditional chinese inn courtyard, "
                "wooden inn building with red lanterns, outdoor seating area with tables, "
                "potted plants and decorative stones, evening atmosphere with warm lighting, "
                "busy with NPC characters, lively atmosphere, "
                "high quality game art, detailed environment illustration, warm inviting mood",
                "low quality, blurry, watermark, text, logo, signature, modern elements, "
                "empty, abandoned, dirty",
                "客栈场景，热闹的傍晚"
            }
        };
        return templates;
    }

    static const std::vector<PromptTemplate>& GetItemIconTemplates() {
        static std::vector<PromptTemplate> templates = {
            {
                "灵茶图标",
                "beautiful game item icon, magical spiritual tea leaves in ornate chinese tea jar, "
                "glowing with soft green energy, detailed porcelain container with cloud patterns, "
                "floating spirit particles around, clean white background, "
                "high quality icon design, centered composition, professional game art",
                "low quality, blurry, watermark, text, logo, signature, dirty container, "
                "broken, deformed, ugly",
                "灵茶物品图标，仙气缭绕"
            },
            {
                "装备图标",
                "beautiful game item icon, elegant chinese style sword with jade hilt, "
                "glowing with spiritual energy, ornate dragon pattern on blade, "
                "clean white background, dramatic lighting, "
                "high quality icon design, centered composition, professional game art",
                "low quality, blurry, watermark, text, logo, signature, dull, "
                "broken blade, ugly hilt",
                "装备图标，中国风宝剑"
            },
            {
                "药材图标",
                "beautiful game item icon, mystical chinese herbal medicine ingredients, "
                "glowing spiritual herbs in traditional medicine jar, "
                "various rare ingredients visible, soft magical glow, "
                "clean white background, professional presentation, "
                "high quality icon design, centered composition, professional game art",
                "low quality, blurry, watermark, text, logo, signature, messy, "
                "rotten ingredients, ugly",
                "药材物品图标，珍贵草药"
            }
        };
        return templates;
    }

    static std::string BuildCustomCharacterPrompt(
        const std::string& gender,
        const std::string& age,
        const std::string& hairstyle,
        const std::string& outfit,
        const std::string& pose,
        const std::string& expression,
        const std::string& accessories,
        const std::string& art_style
    ) {
        return art_style + ", " +
               gender + " character portrait, age: " + age + ", " +
               "hairstyle: " + hairstyle + ", " +
               "outfit: " + outfit + ", " +
               "pose: " + pose + ", " +
               "expression: " + expression + ", " +
               "accessories: " + accessories + ", " +
               "detailed face, beautiful eyes, high quality illustration";
    }

    static std::string BuildCustomScenePrompt(
        const std::string& scene_type,
        const std::string& location,
        const std::string& time,
        const std::string& weather,
        const std::string& atmosphere,
        const std::string& art_style
    ) {
        return art_style + " game background scene, " +
               scene_type + ", " +
               "location: " + location + ", " +
               "time: " + time + ", " +
               "weather: " + weather + ", " +
               "atmosphere: " + atmosphere + ", " +
               "high quality game art, detailed environment illustration";
    }
};

} // namespace CloudSeamanor::prompt_templates