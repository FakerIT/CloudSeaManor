# 参考数据系统（ES3）设计

> 目标：将 `05_参考与第三方资源/02_参考数据` 中的 ES3 存档参考文件，转换为可被 CloudSeaManor 使用的结构化数据输入。

## 1. 输入文件特征

- 文件：`SaveFile.es3`、`saveback.es3`
- 格式：JSON 文本，但采用 ES3 包装
  - 包装节点结构：`{ "__type": "...", "value": <真实值> }`
- 键模式既有结构化键（`Save0`、`家园存档_新版_0`），也有拼接键（`商店日购_槽0_白嫖钥匙_次数`）

## 2. 系统分层（符合 infrastructure 责任）

- 新模块：`infrastructure::ReferenceSaveDataSystem`
  - 文件：
    - `include/CloudSeamanor/infrastructure/ReferenceSaveDataSystem.hpp`
    - `src/infrastructure/ReferenceSaveDataSystem.cpp`
- 责任：
  1. 解析 ES3 文本
  2. 解包 `__type/value`
  3. 归一化字段到统一结构
  4. 输出结构校验报告

## 3. 归一化输出结构

归一化快照顶层：

- `source_path`：源文件路径
- `meta`：
  - `selected_save`
  - `language`
  - `offline_last_timestamp`
- `system_settings`：系统存档配置（来自 `SaveS`）
- `slots[]`（固定 3 槽）
  - `slot_index`
  - `have_save`
  - `deleted_save`
  - `save_payload`（来自 `Save0/1/2`）
  - `home_payload`（来自 `家园存档_新版_0/1/2`）
  - `daily_shop`（预留）
- `global_daily_shop_records[]`
  - 从 `商店日购_槽X_商品_字段` 聚合出的记录
  - 字段：`slot_index`、`item_name`、`field_name`、`value`

## 4. 校验策略

- 结构校验：
  - 根节点必须是对象
  - 必须有 `meta`、`slots`
  - `have_save/deleted_save` 必须是 bool
- 字段校验：
  - `SelectSave`、`语言` 缺失时给出告警
- 输出：
  - `BuildValidationReport()` 返回可读字符串列表

## 5. 与现有系统对接建议

- `SaveSlotManager`：
  - 可读取 `slots[*].save_payload` 进行迁移导入
- `GameRuntime`：
  - 可读取 `meta/system_settings` 对应音量、语言、当前槽位
- 经济/商店子系统：
  - 可消费 `global_daily_shop_records`，统一限购窗口与次数逻辑

## 6. 后续扩展

- 增加导出器：将归一化结果写入 `assets/data/reference/*.json`
- 增加字段映射表（中英文键名统一）
- 增加版本化迁移链（`v1 -> v2 -> v3`）
