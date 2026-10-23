// 원본 10.78의 TypeNames/Deck snapshot writer를 현재 월드와 명시 쓰기 stream에 연결한다.
#pragma once
#include "o/RawMemStream.h"
#include <string_view>

namespace netstorm::o {
// 각 부모의 원본 assert 위치를 구별하며 진단이 복귀하면 원문 처리를 계속한다.
enum class FortSnapshotAssertion { MissingNamesTemplate,NamesNotMine,MissingDeckTemplate,WriteCapacity };
// 파일·현재 월드 효과만 공급하고 해시/루프/필드 쓰기는 부모 본체가 수행한다.
struct FortSnapshotBasicsHooks {
    std::function<std::uint32_t(std::int32_t)> manager; // 같은 this Template의 현재 manager DWORD를 읽는다.
    std::function<void(FortSnapshotAssertion)> assertion; // 선택된 원본 진단이다. 무시한 진단도 해당 callback을 제공한다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> create; // 0044d1b0: 현재 manager에 새 섹션을 만들고 명시 stream 토큰을 반환한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> write; // 0044cce0: 최초 count BYTE의 실제 일반 쓰기다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; // 명시 stream의16BYTE/물리 버퍼를 동기적으로 빌린다. 자체 원본 효과는 없다.
    std::function<std::span<const std::uint8_t>(std::int32_t)> state; // 004bd550: 현재 this의 State를 조회한다. manager.current가 바뀔 수 있다.
    std::function<std::string_view(std::int32_t)> typeName; // 0049a840(index)+4의 현재 원본 NUL 문자열이다.
    std::function<void(std::uint32_t)> refreshDeck; // 0044f7c0: 같은 실제 생산 자료로 덱을 재계산한다.
    std::function<std::uint32_t(std::uint32_t)> getDeck; // 0044f700: 재계산 뒤 현재 Player의 덱 토큰이다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> deck; // 같은 덱의0x160BYTE를 매 조회 시 빌린다.
};
// 004bd6f0: 새 TypeNames→State+17→count BYTE→현재 타입 이름의 signed hash DWORD 순서다.
void SnapshotFortTypeNames(std::int32_t slot,const std::uint32_t& templateOwner,const std::int32_t& typeCount,const FortSnapshotBasicsHooks& hooks);
// 004befd0: 슬롯을 owner0..8로 변환해 덱을 갱신/조회하고 각5BYTE 항목의 앞4BYTE만 저장한다.
void SnapshotFortDeck(std::int32_t slot,const FortSnapshotBasicsHooks& hooks);
}
