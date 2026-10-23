// 원본의 항등9색표와 일반 플레이어 색 띠/다리 특례를 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::o { class RawColorSortPrimary; } // 저장명도에직접접근하는실제raw소유자다.
namespace netstorm::client {
// owner 색 번호0..8 각각의 현재256색 변환표다.0번은 항등이다.
using OwnerColorMaps=std::array<std::array<std::uint8_t,256>,9>;
// 실제005319e8의9×8 DWORD 후보를 제공한다. 반환 저장소는 프로그램 전체 수명이다.
std::span<const std::uint32_t,72> OriginalOwnerColorCandidates();
// 0043b950/0043ba00: 항등 초기화 뒤228..245의 띠와 현재 다리 타입의 색2/7 특례를 적용한다.
// candidates는 실제 DWORD 후보이며 원문처럼 lowBYTE만 변환표에 저장한다.
OwnerColorMaps BuildStandardOwnerColorMaps(std::uint32_t type,std::uint32_t bridgeType,
    std::span<const std::uint32_t,72> candidates=OriginalOwnerColorCandidates());
// 실제43be30/43bc40의 전용5종(111/141/137/94/97)을 ColorSort 명도/8후보로 복원한다.
// original CRT 기본53비트에 맞추며64비트 관측과의 차이는 별도 fixture에 보존한다.
OwnerColorMaps BuildSpecialOwnerColorMaps(std::span<const std::uint32_t,256> logical,std::uint32_t type,
    std::span<const std::uint32_t,72> candidates=OriginalOwnerColorCandidates());
// 실제Client용입력은동일raw+8저장double이다.기존logical overload는독립adapter로만남긴다.
OwnerColorMaps BuildSpecialOwnerColorMaps(const o::RawColorSortPrimary& primary,std::uint32_t type,
    std::span<const std::uint32_t,72> candidates=OriginalOwnerColorCandidates());

}
