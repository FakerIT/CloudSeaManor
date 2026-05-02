const fs = require('fs');
const path = 'd:\\CloudSeaManor\\02_工程主代码\\CloudSeaManor\\include\\CloudSeamanor\\app\\GameApp.hpp';
let content = fs.readFileSync(path, 'utf8');

const old = `    // 加载映射表
    [[nodiscard]] SpriteMapping* GetCurrentMapping() { return &current_mapping_; }

private:`;

const newText = `    // 加载映射表
    [[nodiscard]] SpriteMapping* GetCurrentMapping() { return &current_mapping_; }

    // 解析 CSV 行
    static std::vector<std::string> ParseCsvLine_(const std::string& line);

    // 去除字符串首尾空白
    static std::string Trim(const std::string& str);

private:`;

if (content.includes(old)) {
    content = content.replace(old, newText);
    fs.writeFileSync(path, content, 'utf8');
    console.log('SUCCESS');
} else {
    console.log('PATTERN NOT FOUND');
}
