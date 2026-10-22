// 원본 AI 다리의 실제 모양·점유·끝점 평가와 호출당8개 후보 검색을 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawPathRuntime.h"
#include "o/CanonDecoder.h"
#include "o/SpStore.h"

namespace netstorm::o {
//0040f880/0040f910이 읽는 배치 후보16BYTE의 호스트 표현이다.
struct AiBridgePlacement {
    RawPathPoint point; //첫 칸의float XY이며 별도 반올림하지 않는다.
    std::uint32_t pattern; //현재8개 재고에서 읽은 전체DWORD 패턴 번호다.
    std::uint32_t direction; //원본0/2/4/6 회전 값이다.
};
//0040f400/0040f880/0040f910/004144b0의 실제 계산을 같은 본체·프레임·spot·난수에 적용한다.
class RawAiBridgeSearch {
public:
    static constexpr std::size_t kShapeBytes=0xbc; //8×8 BYTE grid+10개12BYTE 끝점+DWORD count다.
    RawAiBridgeSearch(RawAiBrainMemory& memory,std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> spots,const std::uint32_t& bridge,GameRandom& random); //현재 bridge 전역/프레임과 공용 난수를 빌린다.
    void Shape(std::uint32_t pattern,std::uint32_t direction,std::span<std::uint8_t> raw) const; //caller의 grid/미사용 끝점 여백을 지우지 않고 원본처럼 실제 칸과 count만 갱신한다.
    bool Clear(const AiBridgePlacement& placement) const; //실제 decoder 순서의 모든 칸에서spot BYTE가0인지 확인한다.
    std::int32_t Score(const AiBridgePlacement& placement,std::int32_t targetX,std::int32_t targetY,std::span<const std::uint8_t> shape,std::uint32_t excluded,RawPathPoint& endpointScratch) const; //연결에 쓴 끝점을 제외하고 strict 최저점에서만 결과XY를 갱신한다.
    bool Search(std::uint32_t token,RawPathPoint& endpointScratch) const; //호출당 후보×재고×회전8개를 처리하고 다음 호출에 원본 진행 필드를 이어 쓴다. scratch는 원본 caller의 미초기화 임시XY 입력/출력이다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //살아 있는 정확한0xb08 본체만 조회한다.
    CanonDecoder Decoder(const AiBridgePlacement& placement) const; //현재bridge 프레임과 실제26개 패턴을 조회한다.
    RawAiBrainMemory& memory_; //같은 실제 Brain의 유일한 바이트 저장소다.
    std::span<const RiftTypeFrames> frames_; //현재 TYPE 순서의 실제 코드 표다.
    std::span<const std::uint8_t> spots_; //현재65536BYTE 점유 지도다.
    const std::uint32_t& bridge_; //005411a0의현재 전체DWORD 번호다.
    GameRandom& random_; //00532710과 같은 게임 전역 난수 상태다.
};
}
