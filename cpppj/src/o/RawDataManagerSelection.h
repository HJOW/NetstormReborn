// DataManager의 같은 목록/current를 사용하는 검색44ce60·선택44ceb0이다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
namespace netstorm::o {
// 원본 진단 두 종류는 진단 뒤 현재 포인터를 다시 읽는 지점도 구분한다.
enum class DataManagerSelectionAssertion { MissingCurrent,EmptyRead };
// 노드는 호스트 포인터를DWORD로 자르지 않고 현재60BYTE의 불투명 핸들로 찾는다.
struct DataManagerSelectionHooks {
    std::function<std::span<std::uint8_t>(std::uint32_t)> stream; // 현재 노드의16BYTE MemStream·이름40BYTE·next DWORD다.
    std::function<int(std::string_view,std::string_view)> compare; // 실제 CRT 경계다. 생략하면 공식 ASCII 이름 비교를 사용한다.
    std::function<void(DataManagerSelectionAssertion)> assertion; // 선택된 원본 진단만 호출하며 이후 current를 다시 읽는다.
};
// 공식 섹션 이름의 ASCII 대소문자 비교다. 비ASCII CRT 로캘 처리는 명시적으로 거부한다.
int CompareDataManagerSectionNames(std::string_view first,std::string_view second);
// head에서 첫 이름 일치를 찾으며 실패 시 manager.current는0이다. 각 비교 뒤 현재 포인터를 재조회한다.
std::uint32_t FindDataManagerSection(std::span<std::uint8_t> manager,std::string_view name,const DataManagerSelectionHooks& hooks);
// 검색/필요 진단 뒤 현재 노드 pos만0으로 쓴다. writable이0이면 빈 버퍼를 진단한다.
std::uint32_t SelectDataManagerSection(std::span<std::uint8_t> manager,std::string_view name,std::int32_t writable,const DataManagerSelectionHooks& hooks);
}
