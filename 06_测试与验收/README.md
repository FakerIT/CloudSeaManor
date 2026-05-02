# 测试与验收

本目录包含云海山庄项目的测试用例、验收报告和性能报告。

## 目录结构

```
06_测试与验收/
├── 测试用例/          # 单元测试、集成测试、QA 测试用例
├── 验收报告/         # 各版本的验收测试报告
└── 性能报告/         # 性能测试报告（FPS、内存、启动时间）
```

## 运行测试

```bash
# CMake 构建时启用测试
cmake -S 02_工程主代码/CloudSeaManor -B 02_工程主代码/CloudSeaManor/build -DBUILD_TESTING=ON
cmake --build 02_工程主代码/CloudSeaManor/build --config Debug

# 运行测试
ctest --test-dir 02_工程主代码/CloudSeaManor/build --output-on-failure
```

## 测试覆盖率目标

| 层次 | 覆盖率目标 |
|------|-----------|
| domain | 80% |
| infrastructure | 60% |
| engine | 30% |

## 相关文档

- 测试框架：Catch2（`tests/catch2_main.cpp`）
- 测试用例：`02_工程主代码/CloudSeaManor/tests/`
- 工程规范：`02_工程主代码/CloudSeaManor/docs/ENGINEERING_STANDARDS.md`
