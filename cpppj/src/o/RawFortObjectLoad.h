// Fort의 저장 객체·내용물 적용과 Terr→Chaff 실제 순회를 같은 raw 월드에 연결한다.
#pragma once
#include "o/Template.h"
#include "o/RawContainedCreate.h"
#include "o/RawSquidFinder.h"
#include "o/SquidReward.h"

namespace netstorm::o {
// 객체/상위 로더가 매 분기에서 다시 읽는 원본 전역이다. 미션 기본 소유자는 Load의 Template 칸으로 전달한다.
struct FortObjectLoadState {
    std::uint32_t version{},loadingDepth{},scope{}; // 005c89c0/005c89b8/005c89b0: 판본·로딩 깊이·Terr1/Chaff2다.
    std::uint32_t bridgeType{82},islandType{94},largeIslandType{95},noIslandType{157}; // 실제 PE의 다리/island/islandStalag/noIsland 전역이다. 지면isle97/98과 구별한다.
    std::uint32_t ownerSavedMask{0x5d77cf00},ownerClearedMask{0x10002000},fortKeepMask{0x40480002}; // 두 PE의 초기 mask다.
    bool special{},mission{},fort{},chaffMode{},fortDestroyBlocked{}; // 편집/미션·Fort/Chaff 허용·Fort 삭제 억제 전역이다.
    std::int32_t chunksX{16},chunksY{16}; // 현재 원본 청크 지도 크기다. 각 행/열에서 현재 값으로 비교한다.
    std::uint32_t battleSavedMask{0x40480002}; //0054263c: 저장 시 현재 flags2와 비교하며 fortKeepMask와 다른 전역이다.
};
// 계산을 기본값으로 대체하지 않는 실제 가상/월드 경계다. 각 반환값은 현재 raw/type에 적용된다.
struct FortObjectLoadHooks {
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 실제 factory(type,0)다.
    std::function<void(Sid)> notifySurface; // WORD 지정 뒤 현재 다리 타입이면 표면 알림이다.
    std::function<void(Sid,std::uint32_t)> setState,setOwner; // 가상 +0x84/+0x74이며 사제 파생 규칙도 분배한다.
    std::function<bool(Sid)> hasHitPoints; // 실제 dais/abstract·flags1 판정이다.
    std::function<std::int32_t(Sid)> maxHitPoints,virtualCount; // 현재 최대 HP·가상 +0x80이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 가상 +0x90이며 저장 객체는 flags9다.
    std::function<void(Sid,std::uint32_t)> destroy; // Fort 편집 후처리의 가상 Destroy0다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> createContent; // 실제 004afa70다.
    std::function<std::int8_t(std::int32_t,std::int32_t)> chunkRegion; // 원본 청크 지도 BYTE를 signed로 읽는다.
    std::function<bool(std::int8_t,std::uint32_t,std::int32_t)> territoryAllowed; // 0046f200의 지역/소유자/Terr 조건이다.
    std::function<void(std::uint32_t)> loadingDepthChanged,finishOwner; // 동일 사제/표면 모듈의 깊이 동기화·0040e1a0 후처리다.
};
// 값형 Fort 레코드의 데이터만 사용하며 결과는 실제 SID/해시/타입/난수에 쓴다.
class RawFortObjectLoad {
public:
    // 모든 경계는 필수다. types/frames/state/pool/hash/random은 이 객체보다 오래 살아야 한다.
    RawFortObjectLoad(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,GameRandom& random,FortObjectLoadState& state,FortObjectLoadHooks hooks);
    // 004bdc60/CD00428320: 청크 시작 칸에 position의 nibble을 더하고 초기 필드→소유자/HP/프레임→Pop9→내용물을 적용한다.
    Sid Object(const FortObject& object,std::int32_t originX,std::int32_t originY,std::uint32_t defaultOwner) const;
    // 004bd130/CD00426dd0: 목록의 attach flags 변경은 뒤의 형제에도 남으며 중첩 호출에서는 값으로 전달한다.
    bool Contents(Sid parent,const FortContentList& contents,std::uint32_t flags=9) const;
    // 004be160/CD004288a0: 청크 좌표의 각 성분을 16배하여 저장 순서대로 실제 객체를 적용한다.
    void Chunk(const FortChunk& chunk,std::int32_t x,std::int32_t y,std::uint32_t defaultOwner) const;
    // 004be610/CD00428d50: Terr[start,end) 뒤 허용 시 Chaff, 각 섹션은 y 바깥/x 안쪽 순서다.
    // Template 칸1..8은 해당 기본 소유자다. 패치판의 그 밖 칸은0이며 CD판은 원본 assert를 예외로 거부한다.
    void Load(const FortTemplate& fort,std::int32_t templateSlot,std::int32_t start=0,std::int32_t end=20) const;
    // 실제 동일 풀 어댑터 검사에 사용하는 참조다.
    const SidPool& Pool() const;
private:
    // 현재 raw 타입과 그 타입 표를 읽는다. callback 뒤 별도 사본을 재사용하지 않는다.
    const RiftTypeRecord& Type(Sid sid) const;
    // 현재 프레임 필드는 패치 DWORD/CD BYTE이며 저장 프레임은 가상 SetFrame을 호출하지 않는다.
    void Frame(Sid sid,std::uint32_t value) const;
    // 실제 WORD 지정은 현재 다리 타입이면 알림을 요청한다.
    void Word(Sid sid,std::uint32_t value) const;
    // 004b2070(x,y,island genus2)의 일반 finder 첫 통과 대상이다.
    Sid Terrain(float x,float y) const;
    // Fort의 고정 P/P·wildcard·quartile 없음 요청만 복원한다. 최대256 후보와 같은 전역 난수를 사용한다.
    std::int32_t RandomFrame(Sid sid) const;
    // 한 섹션의 청크 선택과 실제 적용이다. 저장 청크 수가 부족하면 남은 정상 효과 뒤 예외로 거부한다.
    void Section(const FortChunkSection& section,std::uint32_t owner,std::int32_t territory) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;/* hash_: 좌표별 객체 후보를 찾는 공간 해시. */ const SquidHash& hash_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;/* frames_: 타입별 프레임 자료. */ std::span<const RiftTypeFrames> frames_;
    // random_: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom).
    GameRandom& random_;/* state_: 객체/상위 로더가 매 분기에서 다시 읽는 원본 전역이다. 미션 기본 소유자는 Load의 Template 칸으로 전달한다 (FortObjectLoadState). */ FortObjectLoadState& state_;/* hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (FortObjectLoadHooks). */ FortObjectLoadHooks hooks_;
};
// 실제 factory/HP 조회/contained Form 생성만 연결한다. 사제 소유자·가상 상태/Pop/삭제는 월드 분배 경계를 유지한다.
FortObjectLoadHooks MakeFortObjectLoadHooks(const SidPool& pool,SquidFactory& factory,const SquidReward& hp,
    const RawContainedCreate& contained,FortObjectLoadHooks hooks);
}
