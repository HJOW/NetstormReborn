// 공통 Squid 소유자 지정의 현재 모드/TYPE와 부분 실패를 원문 순서로 적용한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 가상 소유자 재정의의 base 몸체이며 Form 부착과 다른 ret4 계약이다.
struct CommonOwnerHooks { // 선택 위치에서만 현재 장치를 요구한다.
    std::function<bool()> fort,battle,challenge; // 같은 현재 모드 전역을 각 원문 위치에서 읽는다.
    std::function<std::uint32_t(std::uint8_t)> genus; // 그 위치의 현재 TYPE BYTE로 같은 TYPE 표를 읽는다.
    std::function<void(std::uint8_t,Sid)> remove,append; // 현재 소유자 BYTE의 실제 작업장 목록을 선택한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 진단이 정상 복귀하면 원래 입력의 하위 BYTE 쓰기를 계속한다.
};
void SetCommonSquidOwner(SidPool& pool,Sid sid,std::uint32_t player,const CommonOwnerHooks& hooks); //004adf00/CD004aefa0의 범위 진단→제거→BYTE→현재 재판정→추가다.
}
