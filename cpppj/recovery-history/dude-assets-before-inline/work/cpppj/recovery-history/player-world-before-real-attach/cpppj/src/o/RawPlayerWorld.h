// 실제 Player152의 첫 생성·PostPop·Technology SID 조회를 같은 월드 기록에 연결한다.
#pragma once
#include "o/SquidPostPop.h"
#include <functional>
#include <span>

namespace netstorm::o {
// 원본 Player 생성과 명시 owner Technology 조회의 진단 종류다.
enum class PlayerWorldAssertion {
    PlayerIdKnown, // current PlayerKnown이0일 때 원본 assert를 전달한다.
    PlayerAlreadyExists // 같은 좌표의 현재 playerType가 이미 존재할 때 원본 assert를 전달한다.
};
// 두 owner를 합치지 않고 원본 getter/첫예약이 사용하는 현재 전역을 공유한다.
struct PlayerWorldState {
    std::uint32_t playerKnown{}; // PATCH595344/CD50f828: 현재 localPlayer 번호가 알려졌는가다.
    std::uint32_t battleOwner{1}; // PATCH540c70/CD50f6c8: CurrentTechnology와 최초 local예약의 현재 owner다.
    std::uint32_t templateOwner{1}; // PATCH540cac/CD50f6d4: 저장 Template 선택용 별도 owner이며 이 네 몸체는 쓰지 않는다.
    std::uint32_t playerType{152}; // PATCH5412b8/CD51cba4: Create/Find가 매 호출 읽는 실제 playerType 전역이다.
    bool fortMode{}; // PATCH594fb8/CD540a1c: local 최초 Regular1을 생략하는 조건이다.
    bool special{}; // PATCH55a480/CD537e8c: 생성 y=10 및 최초 직접 frame1 조건이다.
};
// 실제 Factory/Owner/Pop/예약을 빠짐없이 연결한다. 선택 효과를 기본값으로 대체하지 않는다.
struct PlayerWorldHooks {
    std::function<void(PlayerWorldAssertion)> assertion; // 원본 assert 후 계속 수행하는 진단 경계다.
    std::function<Sid(float,float,std::uint32_t)> find; // PATCH4b1fa0/CD4eb4b0의 같은 Pool/Hash 현재 타입 검색이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 같은 실제 Factory의 Create(type,flags2)다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 가상+74의 실제 SquidOwner를 호출한다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 가상+90의 실제 공간·Player PostPop·표시·장부다.
    std::function<bool()> reserveRegular; // 원본 new(0x28)의 성공/실패를 보존한다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // event1/payload5.0f의 실제 Regular/동일 Host 생성이다.
};
// 현재 Player 9기록에 Technology parent SID를 직접 기록하며 별도 Player 복사본을 만들지 않는다.
class RawPlayerWorld {
public:
    // 동일 Pool/공통Post/9기록/현재 전역을 참조하며 모듈보다 해당 저장소가 오래 살아야 한다.
    RawPlayerWorld(SidPool& pool,SquidPostPop& base,std::span<std::uint8_t> playerRecords,
        PlayerWorldState& state,PlayerWorldHooks hooks);
    // PATCH490250/CD407400: Find→Factory(flags2)→Owner→직접frame0→가상Pop0을 순서대로 실행한다.
    Sid Create(std::int32_t owner) const;
    // PATCH4905d0/CD407cc0: 현재 owner의SID→조건부예약→special 직접frame1→비가상공통Post다.
    void PostPop(Sid sid,std::uint32_t flags) const;
    // 가상 분배기가 실제 Player VT인지 확인한다. 타입 번호만 보고 다른 몸체를 덮지 않는다.
    bool Handles(Sid sid) const;
    // Player 기록/공통Post와 선택된 실제예약을 공간 변경 전에 검사한다.
    void Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop=nullptr) const;
    // PATCH48fc60/CD406d90: PlayerKnown0이면0, 그렇지 않으면 현재 battleOwner의 실제 DWORD를 읽는다.
    std::uint32_t CurrentTechnology() const;
    // PATCH48fc90/CD406dc0: PlayerKnown 진단 후 명시 owner의 실제 DWORD를 읽는다.
    std::uint32_t Technology(std::int32_t owner) const;
    // 월드/분배기의 잘못된 다른 Pool 연결을 거부할 때 사용한다.
    const SidPool& Pool() const;
private:
    // 실제9기록 안의 Technology DWORD 필드 위치를 판본별로 계산한다.
    std::size_t TechnologyOffset(std::uint32_t owner) const;
    SidPool& pool_; // 실제 Factory/Pop/Host와 같은 살아 있는 raw Pool이다.
    SquidPostPop& base_; // 가상 재분배 없이 실제 마지막 공통 장부 몸체를 실행한다.
    std::span<std::uint8_t> players_; // Money/Meta/AI와 공유하는 실제 현재9개 Player BYTE다.
    PlayerWorldState& state_; // 현재 PlayerKnown·두 owner·fort/special을 호출마다 재조회한다.
    PlayerWorldHooks hooks_; // 원본 필수 하위 생성/공간/예약/진단의 실제 연결이다.
};
}
