// 신전의 실제 가상+0x74를 같은 Player 기록·섬·지역·공통 Owner에 연결한다.
#pragma once
#include "o/SquidOwner.h"

namespace netstorm::o {
// 원본 신전 소유자 몸체가 호출하는 실제 하위 경계다.
struct VortexOwnerHooks {
    std::function<std::int32_t(float,float)> islandNumber; // 현재 좌표의 지면 지역 번호를 조회한다.
    std::function<bool(std::int32_t)> islandExists; // 같은 IslandList의 현재 존재DWORD를 검사한다.
    std::function<void(std::int32_t,std::uint32_t)> setIslandOwner; // 실제 IslandList::Owner의 목록/Player 전파를 실행한다.
    std::function<void(float,float)> regionOwnership; // 실제 지역 투표·지형 도장을 mode0으로 요청한다.
    std::function<void()> refreshProduction; // 마지막 실제 생산 갱신0047d890/CD004773a0을 호출한다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> missingIsland; // 현재 SID/지역/새owner의 진단을 보고한 뒤 계속한다.
};
// PATCH004dc4e0/CD0044f2a0의 실제 신전 Owner 순서와 판본 차이를 복원한다.
class RawVortexOwner {
public:
    // Player는 같은9기록이며 outposts는 실제 별도 목록이다. 모든 참조는 이 모듈보다 오래 살아야 한다.
    RawVortexOwner(SidPool& pool,SquidOwner& base,std::span<std::uint8_t> players,
        SquidPostPopList& outposts,std::uint32_t& revision,VortexOwnerHooks hooks);
    // 두 PE의 실제 신전 가상 표+0x74인지 확인한다. 타입 이름으로 다른 몸체를 대체하지 않는다.
    bool Handles(Sid sid) const;
    // 이전 신전 해제→섬/지역/패치목록→공통 Owner→현재 신전 기록→생산→패치 revision 순서다.
    void Set(Sid sid,std::uint32_t owner) const;
    // 다른 월드의 분배기를 연결하지 않도록 같은 Pool과 Player 주소를 제공한다.
    const SidPool& Pool() const;
    std::span<std::uint8_t> Records() const;
private:
    // 초기 신전 또는 복사DWORD의 현재 Player 저장 주소다.
    std::size_t TempleField(std::uint32_t owner,bool copy=false) const;
    SidPool& pool_; // 실제 Factory/Pop의 살아 있는 슬롯 저장소다.
    SquidOwner& base_; // 기존 독립 검증된 비가상 공통 Owner 몸체다.
    std::span<std::uint8_t> players_; // 생성·후처리·경제·AI와 공유하는 실제9 Player BYTE다.
    SquidPostPopList& outposts_; // 패치 추가 지역 갱신이 매 반복 읽는 별도 현재 목록이다.
    std::uint32_t& revision_; // PATCH5949d4의 생산 갱신 뒤 증가하는 현재DWORD다. CD는 쓰지 않는다.
    VortexOwnerHooks hooks_; // 원본 필수 하위의 실제 공유 저장소와 소비자다.
};
}
