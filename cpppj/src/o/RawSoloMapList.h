// 원본 단독/편집 맵 목록의 현재 캐시와 행 표시 규칙을 보존한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::o {
// 005cab90/005cac58/005cad20의 같은 50칸 저장소다. 빈 문자열도 존재하는 포인터와 구별한다.
struct SoloMapListState {
    std::array<std::optional<std::string>,50> names; // 실제 strdup 문자열의 소유자이며 null은 없는 포인터다.
    std::array<std::uint32_t,50> flags{}; // 원본 읽기 실패/손상/다른 구독자/예약 비트를 그대로 보관한다.
    std::int32_t count{}; // 원본의 부호 있는 현재 목록 개수다.
    std::uint32_t dirty{}; // 00542678의 현재 캐시 정리 요청이다.
};
// 실제 파일 열거/Template 읽기와 표시 장치를 호출 순서대로 제공한다.
struct SoloMapListHooks {
    std::function<std::int32_t()> mayEditMissions; // 현재 설정을 캐시 검사보다 먼저 읽는다.
    std::function<std::vector<std::string>()> enumerate; // 원본 파일 열거 순서의 확장자를 제외한 이름이다.
    std::function<std::uint8_t(std::string_view)> kind; // 00460c40의 현재 State+17 BYTE다.
    std::function<void()> construct; // 임시 Template20 생성과 isMine=1 쓰기 경계다.
    std::function<void(std::string_view)> load; // 현재 이름의 PathSpec과 실제 Template Load 경계다.
    std::function<bool()> present; // 0041aad0의 현재 Template 첫 DWORD 조회다.
    std::function<std::string()> subscriber; // 004bd4d0의 실제 Subscriber 이름이다.
    std::function<std::uint8_t()> stateByte; // 004bd550 이후 현재 State 첫 BYTE다.
    std::function<std::string()> userName; // 00540ca8의 현재 비교 대상 문자열이다.
    std::function<std::uint32_t()> reserved; // 004bd340의 진단/반환 경계이며 원본 정상 반환은0이다.
    std::function<std::string(std::string_view)> translate; // 손상 표시에만 조회하는 현재 번역이다.
};
// 004cb210이 현재 메뉴에 전달하는 한 행이다.
struct SoloMapRow {
    std::string label; // 원본 색 제어와 손상 접미사를 포함한 좁은 문자열이다.
    std::string name; // 선택 명령의 현재 파일 이름 인자다.
    bool checked{},enabled{}; // 현재 이름 일치와 원본 flags8의 선택 관문이다.
};
// 파일 종류 필터·캐시·현재 필드 재조회와 행 표시를 복원한다.
class RawSoloMapList {
public:
    RawSoloMapList(SoloMapListState& state,SoloMapListHooks hooks); // 생성만으로 파일/설정을 읽지 않는다.
    std::int32_t Refresh(std::int32_t mode) const; // 004cafd0의 최대50개 목록을 갱신하고 현재 개수를 반환한다.
    std::optional<SoloMapRow> Row(std::int32_t index,std::string_view current,std::string_view fallback) const; // 004cb210의 현재 선택/표시다.
private:
    std::size_t Index(std::int32_t value) const; // 원본 배열 밖의 손상된 입력만 호스트에서 거부한다.
    const std::string& Name(std::int32_t value) const; // 현재 포인터를 매 읽기 위치에서 재조회한다.
    SoloMapListState& state_; // Client 수명의 단일 캐시를 빌린다.
    SoloMapListHooks hooks_; // 선택된 파일/장치 경계이며 빈 성공으로 대체하지 않는다.
};
}
