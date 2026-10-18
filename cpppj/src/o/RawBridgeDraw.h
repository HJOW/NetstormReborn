// 다리 캡처의 실제 기본 프레임과 기본0인 수명 숫자 표시의 가상 경계를 연결한다.
#pragma once
#include "o/SidPool.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
// 실제5453ac의 기본0이다. 개발용 숫자 표시를 지원할 때도 같은 현재 값을 공유한다.
struct BridgeDrawState {
    std::uint32_t lifeNumbers{}; // 기본0은4223c0이 그림/객체를 읽기 전에 정상 반환하는 분기다.
};
// 같은 raw 풀의 가상+78/+a4를 ShapeToBuffer에 공급하며 현재 프레임을 캐시하지 않는다.
class RawBridgeDraw {
public:
    // 현재 동결은10.78이며 풀과 현재 표시 전역은 이 처리기보다 오래 살아야 한다.
    RawBridgeDraw(const SidPool& pool,const BridgeDrawState& state):pool_(pool),state_(state) {
        if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("다리 캡처 가상 그림 판본 오류");
    }
    // 실제4222f0의 mov eax,[ecx+24]/ret에 해당하며 실제 다리의 현재 DWORD를 반환한다.
    std::int32_t MainFrame(Sid sid) const {
        const auto raw=pool_.Slot(sid);if (Dword(raw,0)!=0x5034c8U) throw std::logic_error("다리 기본 그림의 다른 가상 표");return std::bit_cast<std::int32_t>(Dword(raw,36));
    }
    // 실제4223c0은 기본0일 때 source/x/y를 읽지 않는다. 미복원 개발용 숫자는 빈 성공으로 넘기지 않는다.
    void Additional(Sid,std::int32_t,std::int32_t) const {
        if (state_.lifeNumbers) throw std::logic_error("개발용 다리 수명 숫자 표시 미복원");
    }
private:
    // 비정렬 슬롯 DWORD를 원본 little endian 순서로 읽는다.
    static std::uint32_t Dword(std::span<const std::uint8_t> raw,std::size_t offset) {
        std::uint32_t value{};
        // 네 바이트의 원래 자리를 보존한다.
        for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value;
    }
    const SidPool& pool_; // 같은 실제50바이트 원문이며 복사하지 않는다.
    const BridgeDrawState& state_; // 현재 수명 숫자 전역의 단일 저장소다.
};
}
