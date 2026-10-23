// 원본10.78의004bcea0 내용물 writer를 실제 finder/Form/타입/명시 stream에 연결한다.
#pragma once
#include "o/RawMemStream.h"

namespace netstorm::o {
// 본체가 선택하는 원본 용량/목록 플래그 진단이다. 반환 뒤 이미 쓴 자료를 되돌리지 않는다.
enum class FortContentSaveAssertion { WriteCapacity,ListFlags };
// 각 재귀 호출의 독립 finder 현재/다음 수명이다. current는 원문이 다시 읽는 DWORD다.
struct FortContentSaveCursor {
    std::function<std::uint32_t()> current; // Begin/Next가 반환한 같은 현재 후보다. 다른 콜백 뒤에도 다시 조회한다.
    std::function<void()> next; //004b1b20: 앞서 저장한 next부터 진행하고 같은 현재 후보를 갱신한다.
};
// 각 GetType 호출이 그 시점에 조회한 원본 세 DWORD다.
struct FortContentSaveType {
    std::uint32_t flags1{},contentListFlags{},containerListFlags{}; // 원본 +e8/+100/+fc이며 첫 flags1은 해당 항목 끝까지 보존한다.
};
// 실제 월드 검색/가상 판정과 메모리 빌림만 공급한다. 직렬화 규칙은 본체에서 실행한다.
struct FortContentSaveHooks {
    std::function<FortContentSaveCursor(std::uint32_t)> begin; //004b2270: 호출마다 독립 finder를 만들고 처음 후보까지 진행한다.
    std::function<std::uint32_t()> poolLimit; //005c847c의 현재 물리 SID 개수다. 후보0에서는 조회하지 않는다.
    std::function<bool(std::uint32_t)> robust; // 후보에 대한 실제 가상+60이며 반환 뒤 현재 후보를 다시 읽는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> form; // 현재 후보의+c QA/+12 kind/+14 flags를 같은 raw 슬롯에서 빌린다.
    std::function<FortContentSaveType(std::uint32_t)> type; //0049a840: 캡처한 kind WORD로 현재 타입을 조회한다.
    std::function<FortContentSaveType(std::uint32_t)> parentType; //004ac550: 고정 부모 슬롯의 현재 타입을 조회한다.
    std::function<void(FortContentSaveAssertion)> assertion; // 원본36/278행 진단이며 선택된 경계만 요구한다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; // 보관한 stream 필드/실제 버퍼의 동기적 빌림이다. 원본 외부 효과는 아니다.
};
// 개수 BYTE→robust 통과 항목→QA BYTE/WORD→목록 플래그→중첩 순서로 같은 stream에 쓴다.
// 호출 스택 대신 명시 프레임을 사용하며 순환 부모는 물리 손상으로 거부한다.
void SaveFortContents(std::uint32_t parent,std::uint32_t stream,const FortContentSaveHooks& hooks);
}
