// 스펠의 월드/파편 표시에서만 적용하는 임시 도움말 그림 제외 규칙이다.
#pragma once
#include "o/RiftType.h"

namespace netstorm::client { // 도움말 창과 게임 규칙을 변경하지 않는 표시 경계다.
// 원본10.78도 Edit 비석→스펠 전환 뒤 도움말 그림을 깜빡인다. 2026-10-28 사용자 요청에 따라
// 원본과 다르게 그 그림만 임시로 숨긴다. 프레임 진행·예약·효과·삭제는 원래대로 유지한다.
inline bool SuppressSpellHelpFrame(o::OriginalEdition edition,const o::RiftTypeRecord& type,const o::TypeSpecialFrames& special,std::int64_t frame) {
    return edition==o::OriginalEdition::Patch1078 && (type.flags2&o::TypeFlag2::kBomb)!=0 && special.helpFrame>=0 && frame==special.helpFrame;
}
}
