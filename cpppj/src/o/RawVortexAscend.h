// 신전 Ascend의 반경별 적 자산 삭제와 실제 후보 조건을 같은 raw pool/hash/spot에 연결한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 원본 IslandList의 존재 DWORD/+0xc owner만 공유한다. 섬 배치용 owner/territory 추정으로 대체하지 않는다.
struct VortexAscendIsland {
    std::uint32_t exists{}; // 원본 IslandList의 현재 존재 DWORD다.
    std::uint32_t owner{}; // 원본 섬 owner DWORD다. 0은 소유자 없는 섬이다.
};
// 현재 후보판정 전역이다. 플레이 중 바뀌면 다음 후보부터 현재 값을 읽는다.
struct VortexAscendState {
    std::uint32_t noIslandType{157}; // 현재 기본 표면 noIsland 타입 전역이다.
};
// 섬 조회·버퍼 확보·가상 삭제의 실제 외부 효과다. 어떤 것도 무효 콜백으로 채우지 않는다.
struct VortexAscendHooks {
    std::function<std::int32_t(float,float)> islandNumber; // 현재 객체 좌표의 실제 IslandList getIslandId 조회다.
    std::function<bool()> reserveCandidateBuffer; // 원본 new0x50 확보 성공을 공급한다. 실패시 후보를 쓸 수 없어 거부한다.
    std::function<void()> releaseCandidateBuffer; // 원본 free 경계를 같은 호출 순서에서 실행한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 선택 후보의 실제 vtable+10 삭제를 flags0x100000으로 호출한다.
};
// 실제 신전 event0 본체004db8e0와 후보004db680의 공통 판본 분기를 복원한다.
class RawVortexAscend {
public:
    // pool/hash/spot/types/random/섬목록은 이 객체보다 오래 살아야 하며 surface는 hash.Entries(0) 자체다.
    RawVortexAscend(SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const VortexAscendIsland> islands,
        GameRandom& random,VortexAscendState& state,VortexAscendHooks hooks);
    // 004db680/CD0044e230: 같은 섬·허용genus·spot/표면/flags1 조건·다른 owner를 확인한다.
    bool Candidate(Sid sid,std::int32_t island,std::uint32_t owner) const;
    // 004db8e0/CD0044ee30: count32 초과는64, 최대20개 후보 중 난수 하나를 삭제한다. 최종64/빈후보면 false다.
    bool Tick(Sid temple,std::uint32_t count) const;
    // 0046f480/CD004bdba0: 존재하는 섬 중 현재 owner와 다른 nonzero owner의 개수를 반환한다.
    std::uint32_t CountForeignOwnedIslands(std::uint32_t owner) const;
    // Regular 어댑터가 같은 월드 풀에 연결돼 있는지 확인한다.
    const SidPool& Pool() const;
private:
    // 현재 raw 좌표 WORD/DWORD를 정렬과 무관하게 읽는다.
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const;
    SidPool& pool_; // 현재 후보/부모를 조회하는 단일 raw 풀이다.
    const SquidHash& hash_; // 일반 반경finder와 표면0단계가 함께 읽는 실제 해시다.
    std::span<const std::uint8_t> spots_; // 현재 발자국 spot 비트의 공유 지도다.
    std::span<const RiftTypeRecord> types_; // 현재 genus/flags1 비용 등이 저장된 공유 타입 표다.
    std::span<const VortexAscendIsland> islands_; // 현재 섬 존재/소유자 DWORD를 읽는 공유 목록이다.
    GameRandom& random_; // 삭제 후보 선택이 한 번 소비하는 게임 난수 전역이다.
    VortexAscendState& state_; // 현재 noIsland 타입 전역이다.
    VortexAscendHooks hooks_; // 실제 섬 조회/확보/해제/삭제 경계다.
};
}
