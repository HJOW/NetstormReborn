// 10.78 사제의 제단 자격 wrapper4498e0/희생 요청494eb0을 같은 현재 월드에 연결한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o {
// 실제 하위 몸체이며 callback 뒤 raw owner/로컬 owner를 다시 읽는다.
struct PriestSacrificeRequestHooks {
    std::function<bool(Sid,Sid)> allowed; // RawDaisRegular::CanBridge(dais,priest)의 실제 자격 판정이다.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> remove; // 기존 내용물 조회4b26f0(parent,현재너겟종류,0,1)이다.
    std::function<void(Sid,Sid,std::uint32_t)> command; // 같은 Dais의 실제 BridgeCommand(dais,priest,1)이다. 다리 건설이 아니다.
    std::function<void(std::string_view)> music; // 실제 ClientAudio::RequestMusic의 sacrifice.mus 요청이다.
};
// 인수는 원본 SID 순서이며 다른 판본/누락된 효과를 성공으로 대체하지 않는다.
class RawPriestSacrificeRequest {
public:
    // 풀/현재 타입/로컬 owner와 효과는 객체보다 오래 살아야 한다.
    RawPriestSacrificeRequest(const SidPool& pool,const std::uint32_t& nuggetType,const std::uint32_t& localOwner,PriestSacrificeRequestHooks hooks);
    bool Allowed(Sid dais,Sid priest) const; // wrapper는 같은 현재 Dais 자격 판정 결과를 전달한다.
    void Begin(Sid priest,Sid dais) const; // 너겟 내용물 제거→의식 명령→현재 로컬 사제일 때 음악 순서다.
private:
    const SidPool& pool_; // callback 뒤 사제의 현재 owner BYTE를 읽는 유일한 풀이다.
    const std::uint32_t& nuggetType_; // 원본 DAT5412c0이며 초기값154를 매 요청 현재 값으로 읽는다.
    const std::uint32_t& localOwner_; // 원본 DAT540c70의 현재 로컬 owner다.
    PriestSacrificeRequestHooks hooks_; // 실제 현재 Dais/내용물/장치의 필수 연결이다.
};
}
