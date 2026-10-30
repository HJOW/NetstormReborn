// 전체 세계 간헐천의 원본 청크 범위·분할·현재 Player 예산을 계산한다.
#pragma once
#include "o/RawFortFinishOwner.h"
#include "o/ChunkMap.h"

namespace netstorm::o { // 실제 Session의 지도/옵션/Player를 같은 주소로 빌리는 범위 계산이다.
// 일반 미션 모드와 구별되는 missionPost 및 원문 공유 DWORD다.
struct WorldGeyserBoundsState {
    const bool& missionPost; //00594fa4: 전체 범위에서 지역0/1을 합칠지 매번 읽는다.
    const std::int32_t& mapLimit; //00531928: 원본 점 유효성 검사의 열린 상한이다.
    std::int32_t& spacingMargin; //00542654: 일반 범위가 현재 간격-2를 먼저 기록한다.
};
// 실제 ChunkMap 및 현재 옵션 getter만 외부 경계로 둔다.
struct WorldGeyserBoundsHooks {
    std::function<ChunkRect(std::int32_t)> chunkBounds; //00432210: 현재 섬 존재 진단을 포함한 청크 범위다.
    std::function<std::int32_t()> spacing; //0041ca10: 호출 당시 옵션 BYTE6의 실제 간격이다.
};
// 원문 표의 유효 옵션0~3을 간격3/4/5/6으로 읽는다. 물리 표 밖은 호스트 오류다.
std::int32_t WorldGeyserSpacing(std::uint8_t selection);
//0041c8e0: battle에서 Player1~8의+2c 비영/숨김BYTE0인 수를 n으로 세어(12-n)*n을 반환한다.
std::int32_t WorldGeyserBudget(OriginalEdition edition,bool battle,std::span<const std::uint8_t> players,std::span<const std::uint8_t> options);
// 큰 지도 생성 부모00486440이 사용하는 실제00485bc0/00485cc0이다.
class RawWorldGeyserBounds {
public:
    // 현재 전역은 이 객체보다 오래 살아야 하며 값형 모드/지도 사본을 만들지 않는다.
    RawWorldGeyserBounds(OriginalEdition edition,WorldGeyserBoundsState state,WorldGeyserBoundsHooks hooks);
    // 지역0의16배 범위를 읽고 현재 모드에 따라 지역1 결합 또는 간격 여백을 적용한다.
    FortRandomRectangle Whole() const;
    // 최초 지역0 조회 후 fallback 여부를 결정한다. 게임의3×3 외에도 유효한 양의 분할을 지원한다.
    FortRandomRectangle Partition(std::int32_t cell,std::int32_t grid) const;
private:
    // 원본 DWORD<<4 뒤 signed→float 저장을 네 좌표에 각각 적용한다.
    FortRandomRectangle Cells(std::int32_t region) const;
    // 선택된 현재 간격을 공유 DWORD에 쓴 뒤 원본 signed 칸 여백을 반환한다.
    std::int32_t Padding() const;
    //0040e880/0040e8f0의 입력1~255 clamp·현재 점 유효성·min/max 순서다.
    void Extend(FortRandomRectangle& rectangle,float x,float y) const;
    WorldGeyserBoundsState state_; // 실제 모드/세계 크기/여백의 같은 장기 주소다.
    WorldGeyserBoundsHooks hooks_; // 현재 청크 지도와 옵션 getter만 호출한다.
};
}
