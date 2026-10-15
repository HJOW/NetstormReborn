// 로컬 간헐천이 쓰는 Player의 실제 청크 배치 사각형과 칸 단위 확장을 복원한다.
#pragma once
#include "o/CanonDecoder.h"
#include "o/RawFortFinishOwner.h"
#include "o/TerrainBuilder.h"

namespace netstorm::o {
// 원본 assert 뒤에는 현재 공유 상태로 계속 진행한다. 손상된 host 물리 범위는 별도 오류다.
enum class PlayerGeyserBoundsAssertion {
    InactivePlayer, // Player flags에 everLoggedIn/AI가 없다.
    Metamap, // 원본 PlayerId getter는 metamap 상태를 진단한다.
    NotBattle, // 플레이어별 청크 배치가 전투 모드 밖에서 요청됐다.
    InvalidPlayer, // 배치의 Player 번호가1~8이 아니다.
    MissingFrame // 현재 puzzlePiece의 패턴 셀에 일치하는 프레임이 없다.
};
// 별도 미션/옵션 사본을 만들지 않고 같은 현재 전역을 참조한다.
struct PlayerGeyserBoundsState {
    const bool& battle; // 00594fbc의 현재 전투 모드다.
    const bool& mission; // 00594fa0의 현재 미션 모드다.
    const bool& metamap; // 00594fc8의 현재 메타맵 모드다.
    const std::uint8_t& mapSize; // 현재 옵션 BYTE0052f7e6이며0~3은 간격3/4/5/6이다.
    const std::uint32_t& puzzlePieceType; // 00541204의 현재 영역 패턴 타입이다.
    std::span<const std::uint32_t,9> playerDirections; // 0053181c의 현재 Player별 방향표다.1~8의 초기값은4/0/6/2/7/3/5/1이다.
};
// Player0048f5c0→현재record00432430→배치00432050→청크변환0040d900의 몸체다.
class RawPlayerGeyserBounds {
public:
    // 같은 raw Player9/TYPE/프레임과 실제 ChunkMap을 빌리며 임의 섬 Bounds를 받지 않는다.
    RawPlayerGeyserBounds(const SidPool& pool,const ChunkMap& chunks,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> players,PlayerGeyserBoundsState state,
        std::function<void(PlayerGeyserBoundsAssertion)> assertion);
    // 0048f5c0: 현재 Player flags/region→record→PlayerId→배치→16배→양끝4확장이다.
    FortRandomRectangle Bounds(std::uint32_t player) const;
    // 00432430: y행/x열의 첫 signed섬BYTE 일치에서6BYTE를 복사하고 없으면0레코드다.
    std::array<std::uint8_t,6> RegionRecord(std::int32_t region) const;
    // 00432050: 미션의 실제 패턴/위치 또는 전투의 현재옵션/방향 배치를 계산한다.
    ChunkRect Placement(std::int32_t player,std::span<const std::uint8_t,6> record) const;
    // 상위 바인딩이 실제 월드와 같은 풀/청크 지도를 연결했는지 확인한다.
    const SidPool& Pool() const;
    const ChunkMap& Map() const;
private:
    // 원본 현재 옵션0~3의 실제 정수 간격을 읽는다. 물리 표밖 옵션은 오류다.
    std::uint32_t Spacing() const;
    // 같은 record의 실제 CanonDecoder와 TYPE 발자국에서 배치 폭/높이를 얻는다.
    std::array<std::int32_t,2> Dimensions(std::span<const std::uint8_t,6> record) const;
    const SidPool& pool_; // 판본과 상위의 동일 풀 identity를 유지한다.
    const ChunkMap& chunks_; // 현재 지역 record를 직접 읽는 유일한 실제 지도다.
    std::span<const RiftTypeRecord> types_; // 현재 puzzlePiece 발자국을 읽는 실제 TYPE 표다.
    std::span<const RiftTypeFrames> frames_; // 같은 타입 번호의 실제 Canon 프레임 표다.
    std::span<const std::uint8_t> players_; // 현재 flags와 첫 지역DWORD를 읽는 원문9기록이다.
    PlayerGeyserBoundsState state_; // 현재 모드/옵션/타입/배치 방향의 단일 저장소 참조다.
    std::function<void(PlayerGeyserBoundsAssertion)> assertion_; // 원본 진단의 실제 소비자이며 누락을 허용하지 않는다.
};
}
