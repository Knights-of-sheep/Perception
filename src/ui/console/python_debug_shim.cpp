// MSVC Debug + 发布版 Python（Anaconda）链接兼容层。
//
// 背景：pyconfig.h 在 _DEBUG 下会 #define Py_DEBUG，进而启用 object.h 中
// Py_REF_DEBUG 分支：Py_INCREF/Py_DECREF 会调用 _Py_INCREF_IncRefTotal、
// _Py_DECREF_DecRefTotal、_Py_NegativeRefcount —— 这些符号仅存在于调试版
// Python 库（python313_d.lib），Anaconda 不提供，发布版 python313.dll 亦不导出，
// 导致 MSVC Debug 链接报 LNK2019。
//
// 本文件为上述符号提供空/极简实现，并放在静态库 perception_ui 中：链接器仅在
// 最终可执行文件出现这些未解析符号时才抽取本 obj；Release 构建（无 Py_DEBUG）
// 本文件编译为空，无任何副作用。
//
// 关键点：本文件需定义 Py_NO_ENABLE_SHARED，令 PyAPI_FUNC 退化为普通 extern
//（而非 dllimport），从而可以用普通定义提供这些符号而不会与头文件声明冲突
//（dllimport 声明 + dllexport/普通定义会触发 C2491/C4273 且定义被忽略）。
// 注意：PythonConsole.cpp 不能定义该宏（数据符号无 thunk 别名会 LNK2001），
// 它保持 dllimport 引用 __imp_<name>；链接时由 /alternatename 把 __imp_<name>
// 映射到本文件的普通符号 <name>（见 src/app/CMakeLists.txt）。
// 这些只是引用计数追踪钩子，空实现不影响发布版 python313.dll 内部计数路径。
#define Py_NO_ENABLE_SHARED
#include <Python.h>

#if defined(Py_REF_DEBUG) && !defined(Py_LIMITED_API)
extern "C" {

void _Py_INCREF_IncRefTotal(void) {
    // 发布版 DLL 内部不使用该计数；此处仅保证链接通过。
}

void _Py_DECREF_DecRefTotal(void) {
}

void _Py_NegativeRefcount(const char* filename, int lineno, PyObject* op) {
    // 引用计数异常变负时才会被调用；发布版 DLL 内部路径自洽，无需处理。
    (void)filename; (void)lineno; (void)op;
}

}  // extern "C"
#endif  // Py_REF_DEBUG && !Py_LIMITED_API
