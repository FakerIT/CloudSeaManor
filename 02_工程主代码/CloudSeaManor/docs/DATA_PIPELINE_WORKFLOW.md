# 数据表导出与校验流程

## 目标
- 策划不直接手改运行时 `assets/data/*.csv`
- 表改动在提交前可自动发现格式与引用问题

## 推荐流程
1. 在 Excel/Sheets 中维护源表（列名保持 `PascalCase`）。
2. 导出为 UTF-8 CSV 到临时目录。
3. 执行导出脚本生成 JSON 快照：
   - `python scripts/data/export_tables.py --input <table.csv> --output <table.json>`
4. 执行校验脚本：
   - `python scripts/data/validate_tables.py assets/data`
5. 修复报错后再提交。

## 常见错误与修复
- **`missing required 'Id' column`**：补充 `Id` 主键列。
- **`header not PascalCase`**：统一列名大小写（如 `item_id` -> `ItemId`）。
- **`duplicate Id`**：保证主键唯一。
- **`suspicious reference`**：引用字段中不要带空格，统一使用全局 ID。

## 约束
- 关联字段统一后缀：`Id / Ids / ProfileId / ProfileIds`
- 多值字段优先 JSON 字符串格式
- 新增表必须在 `DATA_TABLE_SCHEMAS.md` 补充 schema
