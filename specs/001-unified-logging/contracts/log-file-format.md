# Contract: 日志文件行格式

**Branch**: `001-unified-logging` | **Date**: 2026-08-23 | **Spec**: [spec.md](../spec.md) | **Data Model**: [data-model.md](../data-model.md)

## 目标

日志文件（`app.log` 及归档 `app.log.N`）与开发控制台共用**同一行格式**（FR-003 + Assumptions"同一级别与格式策略"），保证解析一致。

## 行格式（单条记录 = 一行）

```text
YYYY-MM-DD HH:MM:SS.mmm LEVEL [source] message\n
```

### 字段细则

| 字段 | 示例 | 约束 |
|------|------|------|
| 时间戳 | `2026-08-23 14:30:05.123` | 本地时间，毫秒精度；宽度固定（月/日/时/分/秒补零） |
| 级别 | `INFO` | `DEBUG`/`INFO`/`WARN`/`ERROR`/`FATAL`，固定 5 字符右对齐补空格（`WARN `） |
| 来源 | `[PythonConsole.cpp:240]` | 方括号包裹；C++ 为 `basename:line`；Python 为 `name:lineno`；无来源为 `[core]` |
| 分隔 | ` `（单空格） | 时间戳/级别/来源/消息之间各一个空格 |
| 消息 | `hello` | UTF-8；不含换行符（内嵌换行转义为 `\n` 字面量，保证单条=单行） |

### 转义规则

- 消息内的换行符（`\n`、`\r`）转义为字面量 `\n` / `\r`，保证"单条记录 = 单行"
- 消息内的 `[`/`]` 不转义（来源段有界，不会歧义）

### 示例

```text
2026-08-23 14:30:05.123 INFO  [PythonConsole.cpp:240] Python 解释器初始化成功
2026-08-23 14:30:05.456 DEBUG [logger.cpp:88] emit to sink: FileSink
2026-08-23 14:30:06.001 WARN  [__main__:12] 除法出现除零警告
2026-08-23 14:30:06.700 ERROR [app/main.cpp:66] snapshot save failed: /path (multiline\nsecond line)
```

## 轮转与归档（FR-005）

```text
app.log      当前活动文件
app.log.1    最近归档（轮转后的 app.log）
app.log.2    次近归档
app.log.3    最旧归档（再次轮转时删除）
```

- 阈值：单文件 ≥ 5MB（默认）触发轮转；`maxBackups = 3`
- 滚动顺序：删除 `app.log.3` → `app.log.2`→`app.log.3` → `app.log.1`→`app.log.2` → `app.log`→`app.log.1` → 新建空 `app.log`
- 轮转不丢失当前缓冲（先 `flush` 后 `close` 再滚动；滚动后重新打开追加）
- 应用重启后：继续追加到当前 `app.log`，归档链保持（FR-004）

## 编码

- 文件编码固定 UTF-8，无 BOM
- 追加写，不覆盖历史（FR-004）

## 校验

- 解析器可按"首列 `YYYY-MM-DD HH:MM:SS.mmm ` 前缀"识别记录行，跳过无法匹配的行（容忍外部追加/半行残留，不崩溃）
