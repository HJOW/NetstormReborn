// 실제4ah/4bh 검증 진입과 이전 SID 사이의 free/form 간격 확인을 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::o { // 두 검증 DWORD를 공통 객체 receiver와 같은 장기 owner에서 빌린다.
struct SquidVerificationState {
    std::uint32_t& sid; //0054d4d8은 다음 객체를 덮어쓰지 않게 하는 현재 검증 SID다.
    std::uint32_t& previous; //0054d4dc는 간격 확인의 현재 순회 위치이며 sid0 저장과 별개다.
};
struct SquidVerificationHooks {
    std::function<std::string(Sid)> typeName; // 현재 원문 TYPE+4의 디버그 이름이다.
    std::function<void(std::int32_t,std::string_view)> log; // 같은 수준1d의 실제 VERIFY 로그다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 진단 정상 복귀 뒤 입력 SID를 쓴다.
};
void AdvanceSquidVerification(const SidPool& pool,SquidVerificationState state,const SquidVerificationHooks& hooks); //00432600/CD00487ae0의 wrap15000/이전 간격 부모다.
void BeginSquidVerification(const SidPool& pool,std::span<const std::uint8_t> packet,SquidVerificationState state,const SquidVerificationHooks& hooks); //004326e0의4bh6BYTE이다. 현재 sid를0으로 되돌리지 않는다.
void ReceiveFreeSquidVerification(const SidPool& pool,std::span<const std::uint8_t> packet,SquidVerificationState state,const SquidVerificationHooks& hooks); //00432710의4ah7BYTE이며 간격 확인 뒤 sid만0이다.
}
