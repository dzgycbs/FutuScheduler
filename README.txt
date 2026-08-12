# [项目名] FutuScheduler

## 功能
- 行情订阅并转发到中间件
- 自动定时启停（支持多段交易时间）
- 节假日自动休市

## 交易时间
| 时段 | 启动 | 停止 |
|------|------|------|
| 日盘 | 08:50 | 15:59 |
| 夜盘 | 20:50 | 02:35 |

## 依赖
- C++17
- spdlog
- nlohmann/json

## 编译
```bash
mkdir build && cd build
cmake .. && make -j4