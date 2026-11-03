// 실제10.78 SHP를 이름 선택 뒤 직접 map/view로 보관한다.
#pragma once
#include "client/VFXDraw.h"
#include <filesystem>
#include <string_view>
namespace netstorm::o { class BaseFileSystem; } // 현재 두 디스크/등록 TAFF와 같은 이름 고리를 빌린다.
namespace netstorm::client { // CLI/CD의 값형 SHP 입구는 그대로 유지한다.
ShapeDatabase LoadNativeShapeDatabase(const o::BaseFileSystem& files,const std::filesystem::path& primary,std::string_view literal="_shapes.shp"); // 이름 고리/41ad10 뒤 직접W매핑이며 TAFFpayload를 추출하지 않는다.
}
