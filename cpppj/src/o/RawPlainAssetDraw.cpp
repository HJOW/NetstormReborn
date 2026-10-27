// 기존 독립 캡처로 확인한 일반 유닛/워크샾과 같은 원본 가상 진입을 공유한다.
#include "o/RawPlainAssetDraw.h"
#include "o/RawPlainUnitCapture.h"
#include "o/RawPlainWorkshopCapture.h"
#include <array>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,8> kPlainBuildings{{
    {78,0x50e940},{79,0x50ea08},{139,0x509aa0},{142,0x50ead0},{147,0x50e940},{149,0x50e668},{150,0x50e888},{170,0x509388}
}}; // 세 블로커/데빌메이커/아크 스파이어/성장형/포스 필드의 +78=4acfe0/+a4=41c100이다.
// 이력: 2026-10-27 — 포스 필드170/509388도 원본 공통Draw를 사용해 현재 소유자 색표가 적용되게 했다.
}
// 같은 주소만 가진 다른 TYPE과 타입만 같은 미복원 가상 표는 거부한다.
bool IsPlainAssetDraw(std::span<const std::uint8_t> raw) {
    if (raw.size()<50) return false;if (IsPlainUnitCapture(raw) || IsPlainWorkshopCapture(raw)) return true;
    std::uint32_t table{}; // 비정렬 little endian의 현재 가상 표다.
    for (std::size_t index=0;index<4;++index) table|=std::uint32_t(raw[index])<<(8*index); // 현재 네 바이트만 합친다.
    for (const auto& [type,value]:kPlainBuildings) if (raw[10]==type && table==value) return true; // 정확한 원본 쌍만 일반화한다.
    return false;
}
}
