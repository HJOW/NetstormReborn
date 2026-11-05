// 원본 Tell 메타의 단독/저장 맵 목록 부모와 실제 파일 목록을 연결한다.
#pragma once
#include "client/SoloMapCatalog.h"
#include "client/FortSelection.h"

namespace netstorm::client {
inline constexpr std::uint32_t kTellMenuFlags=0x40000U; // 004c83b0의 Tell 세로 메뉴 생성 플래그다.
inline constexpr std::int32_t kMapListPageRows=50; // 5차: 목록 메뉴가 한 번에 보여 주는 최대 행 수다. 원본 한 화면 분량(두 열 25행)이며 이보다 길면 이전/다음 이동 행으로 스크롤한다.
// 목록 부모가 호출하는 장치다. 반환 개수를 캐시하지 않고 각 성공 행 뒤 현재 값을 읽는다.
struct TellMapMenuHooks {
    std::function<void(std::uint32_t)> beginVertical; // 현재 메뉴 스택에 새 세로 열을 만든다.
    std::function<void()> invalidate; // 현재 목록의 dirty를1로 쓴다.
    std::function<void(bool)> selectFileSpec; // true는SOLOSpec, false는fortSpec을 같은 버퍼에 쓴다.
    std::function<void(std::int32_t)> refresh; // 현재 키/dirty로 실제 목록을 갱신한다. 반환 개수는 사용하지 않는다.
    std::function<std::int32_t()> count; // 행 추가 효과 뒤의 현재 부호 있는 목록 개수다.
    std::function<bool(std::int32_t)> appendRow; // 004cb210처럼 현재 행을 추가한 경우만true다.
    // 5차 스크롤 장치다. 모두 비어 있으면 원본처럼 전체 목록을 한 번에 두 열로 만든다.
    std::function<std::int32_t()> windowFirst; // 스크롤 표시 창의 첫 목록 번호를 읽는다.
    std::function<void(std::int32_t)> setWindowFirst; // 표시 창의 첫 번호를 쓴다(범위 보정 뒤와 새로 열 때의 0).
    std::function<bool()> takeKeepCache; // 스크롤 이동 뒤 재표시이면 true이며 일회성 표시를 소비한다. true이면 목록을 다시 열거하지 않는다.
    std::function<void()> reuseCache; // 캐시 재사용 때 직전 목록 구성이 끝에 건 dirty(다음 새 열기용 재열거 요청)를 지워 같은 목록을 다시 열거하지 않게 한다.
    std::function<void(std::string_view,std::int32_t)> appendScroll; // 이전/다음 이동 행(문구, 이동할 행 수)을 현재 열에 추가한다.
};
// 004c86fe..004c88c5의 공통 메뉴 생성/맵 분기를 수행한다. 비맵도 첫 열 생성 효과 뒤false다.
bool AppendTellMapMenus(std::string_view label,std::uint32_t& menuCount,const TellMapMenuHooks& hooks);
// 같은 실제 파일 캐시/키와 현재 선택 이름을 위 부모의 장치에 연결한다.
TellMapMenuHooks MakeTellMapMenuHooks(SoloMapCatalog& catalog,SoloMapFileSpec& fileSpec,
    std::function<std::string_view()> current,std::function<std::string_view()> fallback,
    std::function<void(std::uint32_t)> begin,std::function<void(const o::SoloMapRow&,std::uint32_t)> add);
}
