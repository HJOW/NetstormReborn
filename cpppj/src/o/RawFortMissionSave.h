// Guide의 현재 Mission과 같은 실제 stream을 원본 순서로 저장한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace netstorm::o {
// 004bcbf0의 null 원문 진단이다. 일반 쓰기 용량 진단은 연결한 MemStream writer가 소유한다.
enum class FortMissionSaveAssertion { NullName };
// 이름/필드 getter는 현재 주소의 순수 조회이며 write/진단 callback만 실제 외부 효과를 낸다.
struct FortMissionSaveHooks {
    std::function<std::uint32_t()> current; //00463a80: Guide+800의 현재 Mission 토큰을 진입/문자열 뒤 두 번 조회한다.
    std::function<std::optional<std::span<const std::uint8_t>>()> name; //같은 Guide의 NUL 포함 현재 원문 범위다.nullopt는004bcbf0의null입력이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> field; //선택한 동일 Mission의+58 sequence/+54 flags를 도달할 때 읽는다.0토큰 물리접근은 장치가 거부한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> write; //0044cce0: 같은 명시stream에 기존 WriteMemStream을 연결한다.호출 중 원문범위를 무효화하지 않는다.
    std::function<void(FortMissionSaveAssertion)> assertion; //Template.cpp155 진단이며 복귀하면 빈 문자열로 계속한다.
};
//004bcbf0: 최초 NUL 길이의 하위WORD를 쓴 뒤 같은 원문주소의 최초 전체길이를 NUL 없이 쓴다.
void SaveFortMissionName(std::uint32_t stream,std::optional<std::span<const std::uint8_t>> name,const FortMissionSaveHooks& hooks);
//00482c40: 선택한 Mission의 sequence DWORD쓰기 뒤 같은 Mission의 현재flags DWORD를 읽어 쓴다.
void SaveFortMissionState(std::uint32_t stream,std::uint32_t mission,const FortMissionSaveHooks& hooks);
//00463a80: 현재 Mission이 없으면 무동작, 있으면 이름→현재 Mission 재조회→필드저장이다.guided 여부를 조건에 넣지 않는다.
void SaveFortMission(std::uint32_t stream,const FortMissionSaveHooks& hooks);
}
