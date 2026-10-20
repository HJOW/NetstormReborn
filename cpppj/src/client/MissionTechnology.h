// 원본 Mission의 techAllowed 목록을 현재 건설/생산의 동일 허용 표에 적용한다.
#pragma once
#include "client/Mission.h"
#include "o/RawTotalMade.h"
#include <functional>

namespace netstorm::client {
// 00482eb0이 도달하는 현재 목록 조회·실제 이름 검색·두 허용 쓰기의 경계다.
struct MissionTechnologyHooks {
    std::function<std::string(std::int32_t)> item; // 같은 mission.techAllowed를 매 인덱스에서 새로 조회한다. 빈 항목에서 종료한다.
    std::function<std::int32_t(std::string_view)> findType; // 0049a860의 대소문자 무시 첫 이름 검색이며 미등록은-1이다.
    std::function<void(std::int32_t)> allowAll; // 004c23c0: 첫255DWORD를 allowed==0의 금지값으로 쓴다.
    std::function<void(std::int32_t,std::int32_t)> allow; // 004c23e0: 타입 한 칸을 쓴다. 범위 거부는 실제 저장소가 담당한다.
};
// allow1에서 시작해 deny/allow는 모드만 바꾸고 all/이름에서만 실제 표를 쓴다. 공백을 새로 다듬지 않는다.
void ApplyMissionTechnology(const MissionTechnologyHooks& hooks);
// 현재 Script/보호 타입 표/건설·생산이 공유하는 실제 TotalMade를 연결한다. 별도 허용 사본을 만들지 않는다.
void ApplyMissionTechnology(MissionScript& script,const o::RiftTypeTable& types,const o::RawTotalMade& totalMade);
void ApplyMissionTechnology(MissionScript& script,const o::RiftTypeTable& types,std::span<std::uint32_t> restrictions); // 월드 설치 전에도 같은 Config와 공통255칸 쓰기를 사용한다.
}
