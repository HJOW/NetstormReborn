// 원문447340의 현재Unpop과Form생성을 같은 실제 커서에 연결한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::client { //편집 모드1의 객체 취득을 기존 커서 수명과 분리해 검증한다.
struct CursorObjectPickupHooks { //각 효과 뒤 현재 실제 슬롯을 다시 읽는다.
    std::function<void(o::Sid,std::uint32_t)> unpop; //가상48(source,0)이며 객체를 삭제하거나 복제하지 않는다.
    std::function<o::Sid(o::Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> create; //현재 TYPE/WORD와80/10/0으로4afa70을 호출한다.
    std::function<void(o::Sid,std::uint32_t)> take; //생성 반환0도 원문처럼447000에 전달하며 flags|100을 사용한다.
};
void PickupObjectForCursor(const o::SidPool& pool,std::uint32_t source,std::uint32_t flags,const CursorObjectPickupHooks& hooks); //호출 순서/폭을 보존하고 선택된 누락 경계에서 선행 효과를 유지한다.
}
