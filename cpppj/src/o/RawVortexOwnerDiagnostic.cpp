// PATCH004dc58f..004dc5f8/CD0044f2a0의 상세 작성과 진단 위치를 보존한다.
#include "o/RawVortexOwnerDiagnostic.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace netstorm::o { // 상세와 진단은 별도 선택 장치이며 첫 기록을 rollback하지 않는다.
namespace { // 원문 필드와 CRT 변환의 실제 폭만 처리한다.
// 비정렬 좌표 DWORD를 little endian으로 읽는다.
std::uint32_t PositionBits(std::span<const std::uint8_t> raw,std::size_t offset) {
    return std::uint32_t(raw[offset])|(std::uint32_t(raw[offset+1])<<8)|(std::uint32_t(raw[offset+2])<<16)|(std::uint32_t(raw[offset+3])<<24);
}
// 원문 CRT ftol의 signed64 절삭 하위DWORD다. 무효 FISTP의 하위DWORD는0이다.
std::int32_t Coordinate(float input) {
    const double value=input; // 단정도 raw 좌표를 값 손실 없이 변환한다.
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 원문 256BYTE 지역 버퍼를 채운 뒤 상세 writer와 assertion을 순서대로 선택한다.
void ReportVortexOwnerMissingIsland(const SidPool& pool,Sid sid,std::int32_t island,std::uint32_t owner,
    std::uint32_t count,const VortexOwnerDiagnosticHooks& hooks) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;const auto raw=pool.Slot(sid); // TYPE/free로 앞선 진단 접두를 취소하지 않는다.
    std::array<char,256> text{}; // 여섯 signed 정수와 원문 형식을 보관하는 실제 버퍼 폭이다.
    const auto length=std::snprintf(text.data(),text.size(),"islandList.totalIslandIds=%d\nislandId=%d\npos=(%d,%d)\noldPlayerId=%d\nnewPlayerId=%d\n",
        static_cast<int>(std::bit_cast<std::int32_t>(count)),static_cast<int>(island),
        static_cast<int>(Coordinate(std::bit_cast<float>(PositionBits(raw,14)))),static_cast<int>(Coordinate(std::bit_cast<float>(PositionBits(raw,18)))),
        static_cast<int>(raw[patch ? 34U : 32U]),static_cast<int>(std::bit_cast<std::int32_t>(owner)));
    if (length<0 || static_cast<std::size_t>(length)>=text.size()) throw std::length_error("신전 Owner 원문 상세 버퍼 범위 오류");
    const auto detail=hooks.detail;if (!detail) throw std::logic_error("신전 Owner 실제 상세 저장 장치 미연결");detail({text.data(),static_cast<std::size_t>(length)});
    const auto assertion=hooks.assertion;if (!assertion) throw std::logic_error("신전 Owner 실제 진단 장치 미연결");
    assertion("0",patch ? "\\Ns\\O\\Vortex.cpp" : "\\ns\\o\\vortex.cpp",patch ? 0x235U : 0x232U);
}
}
