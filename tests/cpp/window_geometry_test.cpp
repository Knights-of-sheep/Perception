// ===== 窗口最大化几何纯函数单测（005-multi-screen-maximize）=====
// 覆盖 FR-001~006 / SC-001~003 的几何计算语义：
//   - FR-001/002：任意屏幕（含负坐标/竖排/不对称排列）最大化落在该屏工作区；
//   - FR-004：窗口中心所在屏即最大化目标屏（跨屏边界按中心归属）；
//   - FR-005/006：目标屏不可用/断开时回退首个屏幕（US3 fallback，T013 扩展）；
//   - US2 支撑：最大化前记录的原几何中心仍命中原屏（还原后仍显示在原屏）。
// 仅依赖 QtCore 类型（QRect/QList/QPoint），无 GUI 平台依赖，红-绿 TDD。
#include "ui/window_geometry.h"

#include <cassert>
#include <cstdio>

using perception::ui::window_geometry::maximizeGeometry;
using perception::ui::window_geometry::resolveTargetScreenIndex;

namespace {

// 断言失败输出上下文后中断（测试可执行文件断言保护见 tests/cpp/CMakeLists.txt）
void expectIndex(int expected, const QList<QRect>& screensGeom, const QRect& frame) {
    const int got = resolveTargetScreenIndex(screensGeom, frame);
    if (got != expected) {
        fprintf(stderr, "resolveTargetScreenIndex expected %d got %d (frame=%d,%d %dx%d)\n",
                expected, got, frame.x(), frame.y(), frame.width(), frame.height());
        assert(false);
    }
}

void expectGeom(const QRect& expected, const QList<QRect>& avail, int index) {
    const QRect got = maximizeGeometry(avail, index);
    if (got != expected) {
        fprintf(stderr, "maximizeGeometry expected (%d,%d %dx%d) got (%d,%d %dx%d)\n",
                expected.x(), expected.y(), expected.width(), expected.height(),
                got.x(), got.y(), got.width(), got.height());
        assert(false);
    }
}

}  // namespace

int main() {
    // ---- FR-004：单屏（窗口中心命中）----
    {
        const QList<QRect> geom{{0, 0, 1920, 1040}};  // 含任务栏的完整几何
        const QList<QRect> avail{{0, 0, 1920, 1000}}; // 工作区
        const QRect frame{100, 100, 800, 600};
        expectIndex(0, geom, frame);
        expectGeom(avail[0], avail, 0);
    }

    // ---- FR-001/002：双屏横排，副屏在右（正坐标）----
    {
        const QList<QRect> geom{{0, 0, 1920, 1040}, {1920, 0, 1920, 1040}};
        const QList<QRect> avail{{0, 0, 1920, 1000}, {1920, 0, 1920, 1000}};
        // 窗口中心落在副屏 → 目标副屏
        const QRect frame{2000, 100, 800, 600};
        expectIndex(1, geom, frame);
        expectGeom(avail[1], avail, 1);
        // 窗口中心仍落在主屏 → 目标主屏
        expectIndex(0, geom, {100, 100, 1600, 800});
        expectGeom(avail[0], avail, 0);
    }

    // ---- FR-001/002：副屏在左（负坐标）----
    {
        const QList<QRect> geom{{-1920, 0, 1920, 1040}, {0, 0, 1920, 1040}};
        const QList<QRect> avail{{-1920, 0, 1920, 1000}, {0, 0, 1920, 1000}};
        const QRect frame{-1600, 100, 800, 600};  // 中心 (-1200, 400) 在左屏
        expectIndex(0, geom, frame);
        expectGeom(avail[0], avail, 0);
    }

    // ---- FR-001/002：竖排，副屏在上（负坐标）----
    {
        const QList<QRect> geom{{0, -1080, 1920, 1080}, {0, 0, 1920, 1080}};
        const QList<QRect> avail{{0, -1080, 1920, 1040}, {0, 0, 1920, 1040}};
        const QRect frame{400, -800, 800, 600};  // 中心 (800, -500) 在上屏
        expectIndex(0, geom, frame);
        expectGeom(avail[0], avail, 0);
    }

    // ---- FR-001/002：不对称多屏（不同分辨率混合）----
    {
        const QList<QRect> geom{{0, 0, 1280, 1024}, {1280, 0, 2560, 1440}};
        const QList<QRect> avail{{0, 0, 1280, 984}, {1280, 0, 2560, 1400}};
        const QRect frame{2000, 200, 1200, 800};  // 中心 (2600, 600) 在大屏
        expectIndex(1, geom, frame);
        expectGeom(avail[1], avail, 1);
    }

    // ---- FR-004：跨屏边界 → 按窗口中心归属（中心在右屏）----
    {
        const QList<QRect> geom{{0, 0, 1920, 1040}, {1920, 0, 1920, 1040}};
        const QList<QRect> avail{{0, 0, 1920, 1000}, {1920, 0, 1920, 1000}};
        const QRect frame{1200, 100, 1600, 800};  // 中心 (2000, 500) 在右屏
        expectIndex(1, geom, frame);
    }

    // ---- FR-005/006：窗口中心未命中任何屏幕（中心落在屏幕间缝隙）→ 回退首屏 ----
    {
        const QList<QRect> geom{{0, 0, 1920, 1040}, {2000, 0, 1920, 1040}};  // 缝隙 80px
        const QList<QRect> avail{{0, 0, 1920, 1000}, {2000, 0, 1920, 1000}};
        const QRect frame{1940, 100, 100, 100};  // 中心 (1990, 150) 在缝隙
        expectIndex(0, geom, frame);
        expectGeom(avail[0], avail, 0);
    }

    // ---- FR-005：目标屏断开（窗口仍在旧副屏位置，屏幕列表已只剩主屏）----
    // 窗口中心落在已断开的副屏坐标 → 未命中任何现存屏幕 → fallback 主屏（US3）
    {
        const QList<QRect> geom{{0, 0, 1920, 1040}};  // 副屏已断开
        const QList<QRect> avail{{0, 0, 1920, 1000}};
        const QRect frame{1950, 80, 800, 600};        // 旧位置仍指向已断开副屏
        expectIndex(0, geom, frame);
        expectGeom(avail[0], avail, 0);
    }

    // ---- FR-005：混合 DPI（不同逻辑分辨率/缩放）----
    // 屏A 1920x1080 @100%（逻辑 1920x1080）；屏B 2560x1440 @125%（逻辑 2048x1152）
    {
        const QList<QRect> geom{{0, 0, 1920, 1080}, {1920, 0, 2048, 1152}};
        const QList<QRect> avail{{0, 0, 1920, 1040}, {1920, 0, 2048, 1112}};
        const QRect frame{2500, 200, 800, 600};  // 中心 (2900, 500) 在右侧高分屏
        expectIndex(1, geom, frame);
        expectGeom(avail[1], avail, 1);
        // 主屏窗口 → 主屏工作区
        expectIndex(0, geom, {100, 100, 800, 600});
        expectGeom(avail[0], avail, 0);
    }

    // ---- US2 支撑：最大化前记录的原几何中心仍命中原屏 → 还原后仍显示在原屏 ----
    {
        const QList<QRect> geom{{0, 0, 1920, 1040}, {1920, 0, 1920, 1040}};
        const QList<QRect> avail{{0, 0, 1920, 1000}, {1920, 0, 1920, 1000}};
        const QRect restored{1950, 80, 800, 600};  // 最大化前记录的位置（原屏）
        expectIndex(1, geom, restored);            // 还原后中心仍属副屏
    }

    // ---- 空列表/空项边界 ----
    {
        const QList<QRect> emptyGeom;
        expectIndex(-1, emptyGeom, {0, 0, 800, 600});
        const QList<QRect> emptyAvail;
        expectGeom(QRect(), emptyAvail, 0);  // 全空 → 空矩形（调用方保留原行为）
        // index 越界 → 回退首个非空项
        const QList<QRect> avail{{0, 0, 1920, 1000}, {0, 0, 0, 0}};
        expectGeom(avail[0], avail, 5);
        // 对应项为空（被断开，仅剩首屏有效）→ 跳过空项
        expectGeom(avail[0], avail, 1);
    }

    fprintf(stderr, "window_geometry_test: all passed\n");
    return 0;
}
