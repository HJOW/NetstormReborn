// Fort 로딩 뒤 소유 레지던스의 자연 반환과 실제 지역/반복 랜덤 배치 흐름을 복원한다.
#pragma once
#include "o/RawFortObjectLoad.h"

namespace netstorm::o {
// 최초 진입만 읽는 모드/기본 지역과 반복 생성 때마다 읽는 현재 타입 전역이다.
struct FortFinishOwnerState {
    bool chaffMode{},special{},fort{}; // 00594fa4/005c85a4/00594fb8 및 대응 CD 전역이다.
    std::array<std::int32_t,9> playerRegions{127,127,127,127,127,127,127,127,127}; // Player+0x78/CD+0x70다. 실제 미션 지역으로 초기화한다.
    std::int32_t invalidRegion{127}; // 두 PE의 invalidRegion 전역 초기값이다.
    std::uint32_t residenceType{89}; // 005411bc/CD0051caa8의 실제 레지던스 타입 초기값이다.
};
// 지역 사각형은 원본 네 float의 저장 순서를 그대로 보관한다.
struct FortRandomRectangle { /* left: 사각형의 왼쪽 경계. */ /* top: 사각형의 위쪽 경계. */ /* right: 사각형의 오른쪽 경계. */ /* bottom: 사각형의 아래쪽 경계. */ float left{},top{},right{},bottom{}; };
// 실제0040db90/CD0042c3d0 가상 배치 경계의 입력이다. 원본 region/type/두 고정 인자와 단정도 좌표다.
struct FortRandomPlaceRequest {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    std::uint32_t type{};/* x: X 좌표. */ /* y: Y 좌표. */ float x{},y{};/* region: 지역. */ std::int32_t region{};/* mode: 동작 분기를 선택하는 모드. */ /* force: 강제. */ std::uint32_t mode{},force{};
};
// 외부 결과를 생략하지 않는다. place가 성공하면 실제 Pop/postPop이 같은 residences 목록에 등록해야 한다.
struct FortFinishOwnerHooks {
    std::function<FortRandomRectangle(std::int32_t)> regionBounds; // 실제0046f130/CD004bd8a0이다.
    std::function<double()> wallSeconds; // 00460d70/CD004011b0의 현재 호출별 double 시각이다.
    std::function<std::uint32_t(GameRandom&,const FortRandomPlaceRequest&)> place; // 실제 placement 전체, local PRNG도 전달한다.
    std::function<void()> invalidType; // AddRandom의 타입0/-1 진단이며 반환값0을 유지한다.
};
// 전체 entry·시각 seed·현재 레지던스 재조회·하위1000회 배치 재시도를 한 월드에서 실행한다.
class RawFortFinishOwner {
public:
    // types/residences/state/pool은 이 객체보다 오래 살아야 한다. residences는 AssetPostPopState.residences 자체다.
    RawFortFinishOwner(const SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopList& residences,
        FortFinishOwnerState& state,FortFinishOwnerHooks hooks);
    // 0040e1a0/CD0042ca70: mode/invalid region이면 자연 반환하고 없으면 실제 region→seed→AddRandom을 반복한다.
    void Finish(std::uint32_t owner) const;
    // 0040ded0/CD0042c7a0: type0/-1 진단·높이 다음 폭의 local PRNG·성공 quantity·1000회 상한·마지막 결과다.
    std::uint32_t PlaceRandom(GameRandom& random,const FortRandomRectangle& rectangle,std::int32_t region,
        std::uint32_t quantity,std::uint32_t type,std::uint32_t force) const;
    // 실제004bcab0/CD004a5a10은 wallSeconds*1013을 CRT 절삭한 DWORD의 SAR4+33배로 local seed를 만든다.
    static std::uint32_t TimeSeed(double wallSeconds);
    // loader hook과 같은 pool인지 검사하는 참조다.
    const SidPool& Pool() const;
private:
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;/* types_: 타입 번호로 속성·플래그를 조회하는 타입 표. */ std::span<const RiftTypeRecord> types_;/* residences_: 거주건물. */ SquidPostPopList& residences_;
    // state_: 최초 진입만 읽는 모드/기본 지역과 반복 생성 때마다 읽는 현재 타입 전역이다 (FortFinishOwnerState).
    FortFinishOwnerState& state_;/* hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (FortFinishOwnerHooks). */ FortFinishOwnerHooks hooks_;
};
// 같은 월드의 Finish 몸체를 Fort loader의 필수 owner 후처리에 연결한다.
FortObjectLoadHooks MakeFortFinishOwnerHooks(const SidPool& pool,const RawFortFinishOwner& finish,FortObjectLoadHooks hooks);
}
