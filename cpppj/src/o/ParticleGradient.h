// 원본 Colorsort.cpp의 RGB 팔레트 색 띠와 Particle.cpp의 색 표 병합을 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::o {
// 원본 팔레트와 색 정렬 배열의 고정 원소 수다.
inline constexpr std::size_t kParticlePaletteColors=256;
// 원본 정렬 항목80바이트 중 색 선택에 쓰는 의미 필드다. HSV/패딩은 선택 결과에 관여하지 않는다.
struct ParticleGradientEntry {
    std::array<double,3> translated{}; // 원본 +18/+20/+28: 시작색에서 뺀 RGB 좌표다.
    std::uint32_t index{}; // +30: 실제 팔레트 색 번호다. 마지막 선택 항목은 끝색 번호로 교체될 수 있다.
    double distance{},position{}; // +38/+40: 색 축으로부터의 제곱 거리와 축 위 투영 비율이다.
    std::uint32_t selected{}; // +48: 선택 표식0/1이다. 중복으로 고른 색도1을 유지한다.
};
// 하나의 색 띠가 가진 전체 정렬 결과와 실제 생성 필드다.
struct ParticleGradient {
    std::array<ParticleGradientEntry,kParticlePaletteColors> entries{}; // 원본 CRT 순서로 정렬한256항목이다.
    std::int32_t from{},to{}; // +5008/+500c: 가장 가까운 시작/끝 팔레트 번호다.
    std::int32_t count{},selected{}; // +5010/+5014: 요청 색 수와 실제로 표시된 서로 다른 선택 수다.
    double threshold{}; // 원본00555e00: 원통 안팎을 나누는 제곱 거리다.
};
// PATCH00439de0/0043ad20: RGBX DWORD 팔레트에서 시작/끝 RGB에 맞는 색 띠를 만든다.
// colors는2..256, width는유한 양수이며 반올림 색 수가256미만이어야 한다. 같은 끝색을 고르는 영길이 축은 거부한다.
// 원본 CRT의53비트 정밀도(023f/027f)의 연산/저장 순서를 사용하며 호출자의 팔레트를 변경하지 않는다.
ParticleGradient BuildParticleGradient(std::span<const std::uint32_t,kParticlePaletteColors> palette,
    std::array<double,3> from,std::array<double,3> to,std::int32_t colors,double width=2.4);
// PATCH00487d20: 최대3개의 연속 색 띠에서 요청 색 수만큼 앞 항목 번호를 차례로 합친다.
// 실제 선택 수와 요청 색 수가 다를 때도 원본처럼 요청 수를 사용한다. 인접 할당 메모리는 포함하지 않는다.
std::vector<std::uint32_t> MergeParticleGradients(std::span<const ParticleGradient> gradients);
// PATCH00457ea0: 실제 PE의 RGB double 상수와8+3색으로 Fiery Tail의11색 표를 만든다.
// 초기 입자가 읽을 수 있는 배열 뒤의 인접 할당 DWORD는 이 반환값에 임의로 추가하지 않는다.
std::vector<std::uint32_t> BuildFieryTailPalette(std::span<const std::uint32_t,kParticlePaletteColors> palette);
}
