// 원문 후단 Hash/Fire/Spot/Board 진단을 현재 원문 배열과 표시 장치로 연결한다.
#pragma once
#include "client/DrawWater.h"

namespace netstorm::client { // 기본 UI나 임의 진단 활성화 없이 현재 플래그만 읽는다.
enum class RendererDiagnosticKind { Hash,Fire,Spot,Board }; // 436450의 원문 네 독립 후단이다.
struct RendererDiagnosticHooks {
    DrawWaterHooks screen; // 원문 clip/rectangle/전체viewport출력은768의 같은 장치를 사용한다.
    std::function<void(std::array<std::int32_t,2>,std::array<std::int32_t,2>,std::uint32_t)> line; //4a23b0의 전체clip/잠금/VFX 선이다.
    std::function<std::span<const std::uint32_t>()> colors; //54d928/531858의 현재9색 배열을 매 조회마다 빌린다.
    std::function<std::uint32_t()> invalidGraph; //540410의 같은 현재 그래프 무효값이다.
    std::function<std::span<const std::int16_t>()> distances; //59514c의 현재실제BFS표이며미할당은빈span이다. 검색을 새로 실행하지 않는다.
    std::function<void(std::uint32_t)> assertion; //원문진단이자연복귀하면계속하며미연결은명시실패다.
};
// 실제 풀/해시/spot/TYPE만 빌리며 표시용 월드나 거리표를 생성하지 않는다.
class RawRendererDiagnostics {
public:
    RawRendererDiagnostics(const o::SidPool& pool,const o::SquidHash& hash,std::span<const o::RiftTypeRecord> types,std::span<const std::uint8_t> spots); //같은 현재 물리를 동기 호출 동안 빌린다.
    void Draw(RendererDiagnosticKind kind,const RawCameraViewState& renderer,const RawCameraViewState& snapshot,std::int32_t width,std::int32_t height,const std::uint32_t& flag,const std::uint32_t& fireMode,const std::int32_t& worldCells,std::uint32_t& background,const RendererDiagnosticHooks& hooks) const; //flag0은 다른 의존성을 조회하지 않는다.
private:
    const o::SidPool& pool_;const o::SquidHash& hash_; //현재 SID50BYTE와 실제네해시를소유자보다짧게빌린다.
    std::span<const o::RiftTypeRecord> types_;std::span<const std::uint8_t> spots_; //현재타입과65536BYTE의동일물리다.
};
bool DispatchRendererDiagnosticKey(InputEvent event,std::uint32_t special,std::uint32_t specialAuxiliary,std::uint32_t& fire,std::uint32_t& spot,std::uint32_t& board,std::uint32_t& redraw,const std::function<void(InputEvent)>& positionLog={}); //원문7/10/13키만선택하며Shift-P는기존역투영/좌표로그를지연호출한다.
}
