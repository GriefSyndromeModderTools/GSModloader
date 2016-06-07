// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#include "resource.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息

#define ISOLATION_AWARE_ENABLED 1
#include <Windows.h>
#include <commctrl.h>

#include <tchar.h>
#include <fstream>

#include "../GSInjector/GSInjector.h"

#include "GSOXPlugin.h"
#include "LinkDialog.h"
