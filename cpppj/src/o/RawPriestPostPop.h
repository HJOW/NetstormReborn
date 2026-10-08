// 사제 postPop에서 carrier 호출 전의 목록·회복 예약·소유자 상태 초기화를 복원한다.
#pragma once
#include "o/RawPriestOwner.h"
#include "o/SquidProcess.h"
#include "o/SquidReward.h"

namespace netstorm::o {
// 회복 Regular의 실제 이벤트 번호다(004950f0 / CD 0040c110).
inline constexpr std::uint32_t kPriestRegenEvent=0x25a;
struct PriestPostPopState {
    SquidPostPopList priests; // 005954c4 / CD 00549188: 중복 없이 등록하는 사제 목록이다.
    std::array<std::uint32_t,kPlayerCount+1> ownerState{}; // 00595428 / CD 005119a0. 칸 0은 건드리지 않는다.
};
struct PriestPostPopHooks {
    std::function<bool(Sid,std::uint32_t)> findRegular; // 실제 종속 체인에서 이미 예약된 회복을 찾는다.
    std::function<bool()> reserveRegular; // 원본 new(0x28)의 성공 여부. 실패하면 HP 조회/생성을 생략한다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 확보 성공 뒤 payload를 계산하여 Regular에 연결한다.
};
class RawPriestPostPop {
public:
    // 풀과 최대 HP 모듈은 같은 월드를 참조해야 하며, 상태와 훅의 참조 대상은 이 객체보다 오래 살아야 한다.
    RawPriestPostPop(SidPool& pool,const SquidReward& hp,PriestPostPopState& state,PriestPostPopHooks hooks);
    // 004950f0 / CD 0040c110의 carrier 진입 전까지만 실행한다. 전체 가상 postPop으로 등록하지 않는다.
    // extra & 9면 아무 효과도 없고, flags & 1일 때만 목록/회복을 예약한다.
    void Prefix(Sid sid,std::uint32_t flags) const;
private:
    SidPool& pool_;
    const SquidReward& hp_;
    PriestPostPopState& state_;
    PriestPostPopHooks hooks_;
};
// 기존 ProcessHost의 실제 찾기/부착에 연결한다. 호스트 메모리 부족은 C++ 예외로 전파한다.
// 원본 new 실패 분기를 재현할 때에는 반환 훅의 reserveRegular를 별도로 주입한다.
PriestPostPopHooks MakePriestPostPopProcessHooks(SquidProcessHost& host);
}
