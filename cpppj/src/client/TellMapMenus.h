// 원본 Tell 메타의 단독/저장 맵 목록 부모와 실제 파일 목록을 연결한다.
#pragma once
#include "client/SoloMapCatalog.h"

namespace netstorm::client {
inline constexpr std::uint32_t kTellMenuFlags=0x40000U; // 004c83b0의 Tell 세로 메뉴 생성 플래그다.
inline constexpr std::uint32_t kReadFortCommand=0x020000caU; // 004cb210의 실제 ReadFort 맵 선택 명령이다.
// 목록 부모가 호출하는 장치다. 반환 개수를 캐시하지 않고 각 성공 행 뒤 현재 값을 읽는다.
struct TellMapMenuHooks {
    std::function<void(std::uint32_t)> beginVertical; // 현재 메뉴 스택에 새 세로 열을 만든다.
    std::function<void()> invalidate; // 현재 목록의 dirty를1로 쓴다.
    std::function<void(bool)> selectFileSpec; // true는SOLOSpec, false는fortSpec을 같은 버퍼에 쓴다.
    std::function<void(std::int32_t)> refresh; // 현재 키/dirty로 실제 목록을 갱신한다. 반환 개수는 사용하지 않는다.
    std::function<std::int32_t()> count; // 행 추가 효과 뒤의 현재 부호 있는 목록 개수다.
    std::function<bool(std::int32_t)> appendRow; // 004cb210처럼 현재 행을 추가한 경우만true다.
};
// 004c86fe..004c88c5의 공통 메뉴 생성/맵 분기를 수행한다. 비맵도 첫 열 생성 효과 뒤false다.
bool AppendTellMapMenus(std::string_view label,std::uint32_t& menuCount,const TellMapMenuHooks& hooks);
// 같은 실제 파일 캐시/키와 현재 선택 이름을 위 부모의 장치에 연결한다.
TellMapMenuHooks MakeTellMapMenuHooks(SoloMapCatalog& catalog,SoloMapFileSpec& fileSpec,
    std::function<std::string_view()> current,std::function<std::string_view()> fallback,
    std::function<void(std::uint32_t)> begin,std::function<void(const o::SoloMapRow&,std::uint32_t)> add);
}
