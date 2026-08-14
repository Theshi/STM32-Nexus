# 三屏 UI 架构 + 集中式屏幕管理器 设计

日期：2026-08-14
平台：STM32F103VET6 + FreeRTOS + LVGL v8 + W25Q64（图标/字库存外部 flash，SPI 读取）

## 背景与约束

- RAM 64KB。当前静态占用 RW+ZI ≈ 52KB，其中 **LVGL 堆 32KB**、FreeRTOS 堆 12KB。
- LVGL 所有控件对象与**图标像素缓冲**都从 32KB LVGL 堆分配。
- 一份图标像素缓冲 ≈ 5KB（alarm 等 5004B，left/right 7204B）。
- 外部字库走 XBF + 回调实时读 flash，不占 LVGL 堆（不在本设计范围内）。
- 已有可复用代码：三个屏幕雏形（Boot_UI / LVGL_CreateMainScreen）、链表 Menu（7 节点环形 + `menu_control.current`）、`ui_icon`/`ui_font` 封装。

## 目标

1. 三个屏幕（BOOT / MAIN / MENU）**按需创建销毁**，任一时刻只有一屏存活。
2. **集中式屏幕管理器**统一所有切换，创建/销毁/图标释放集中管理。
3. 菜单屏由链表 `menu_control.current` 驱动，**三卡片渲染 + 触摸导航**。
4. 内存峰值 = 单屏（< 32KB 堆）。

## 设计

### 1. 三个屏幕

| 屏幕 | 内容 | 生命周期 |
|---|---|---|
| BOOT | LOGO + 进度条 + 自检日志 + 图标（现有 Boot_UI） | 临时，进度完成即销毁 |
| MAIN | 时间/日期 + menu 按钮 + 图标（现有 LVGL_CreateMainScreen） | 按需 |
| MENU | 三张卡片（上/当前/下）+ 返回按钮 | 按需 |

### 2. 集中式屏幕管理器（核心）

```c
typedef enum { SCREEN_BOOT, SCREEN_MAIN, SCREEN_MENU } SCREEN_ID;

void Screen_Switch(SCREEN_ID next);
```

职责三步：
1. **销毁当前屏**：`lv_obj_del` 当前屏对象，并释放该屏所有图标缓冲（补 `ui_icon` 的泄漏）。
2. **创建目标屏**：调用对应创建函数，返回 `lv_obj_t *`（逻辑名 CreateBOOT/CreateMAIN/CreateMENU；对应现有 `Boot_UI`、`LVGL_CreateMainScreen` 的改造版，以及新写的菜单屏创建函数）。
3. **切换**：`lv_scr_load`（可带淡入动画）。

所有切换（boot→main、main→menu、menu→main）只走这一个入口。现有 `Boot_Finish`（删 boot + 建 main + 切屏）收敛进 `Screen_Switch(MAIN)`。

### 3. 菜单屏：链表驱动 + 触摸导航

- **渲染 `Menu_Render()`**：按 `menu_control.current` 画三张卡——
  - 上卡 = `current->prev`，中卡 = `current`，下卡 = `current->next`
  - 每卡 = 图标 + 名字 label
  - 重渲染 = 清旧三卡 → 画新三卡（旧的图标缓冲释放、新的分配）
- **交互**（A 方案：按钮与卡片重合）：三个透明按钮叠在三张卡上
  - 上卡点按 → `Menu_Prev()` → 重新渲染
  - 下卡点按 → `Menu_Next()` → 重新渲染
  - 中卡点按 → `Menu_Select()` → 进入 app
- **返回**：菜单屏"返回"按钮 → `Screen_Switch(MAIN)`。

### 4. 数据流（事件 → 状态 → 重绘）

```
触摸卡片 → Menu_Next/Prev/Select（只改 menu_control.current）
        → Menu_Render()（按 current 重画三卡）
lv_timer_handler() 周期把变化画上屏
```

### 5. 内存预算

- 单屏峰值：MENU 三图标 ≈ 15KB；MAIN 三图标 ≈ 9.6KB。按需模式下任一时刻只有一屏 → 峰值 < 32KB 堆 ✓。
- **硬前提：删屏必须释放图标缓冲**，否则切换几次堆就漏光。

## 待处理/一并修复项

1. **`ui_icon` 图标缓冲泄漏**：每次 `lv_mem_alloc(size)` 从不释放。按需模式下必须在删屏时统一释放（`ui_icon` 配套释放函数，或管理器登记后统一 free）。
2. **`Boot_UI` 返回类型**：当前为 `void` 且不返回 `scr`，管理器需要它返回 `lv_obj_t *` 才能持有/销毁。
3. **`Boot_Finish` 收敛**：并入 `Screen_Switch(MAIN)`。
4. **`ui_menu.h` 待写**：外露 `MENU`/`MENU_CONTROL` 类型、`extern menu_control`、`Menu_Init/Next/Prev/Select` 原型。UI 功能完成后抽取，本次不阻塞。

## 后续扩展（非本次范围）

- 实体 HOME 键一键回初始界面（RAM 宽裕后）。
- 层级菜单（链表 `parent`/`child` 字段接入，做子菜单导航）。
