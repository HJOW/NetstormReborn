// 원본 StyleText 문자열 조각·소유 체인·짧은 스타일 명령과 두 번 순회하는 측정/출력을 복원한다.
#pragma once
#include "client/GumpTextDraw.h"
#include "client/PaletteColors.h"
#include <map>
#include <string_view>
#include <vector>

namespace netstorm::client {
// Text는 소유 문자열, External은 그림 등 파생 조각, Base는 소멸 후 기본 가상 표의 빈 동작이다.
enum class StyleTextFragmentKind { Text, External, Base };
// 원본 +4 next/+8 용도 미확정 DWORD/+c NUL 문자열/+10 문맥이다. 호스트 배치는 원본 주소와 무관하다.
struct StyleTextFragment {
    StyleTextFragmentKind kind{StyleTextFragmentKind::Text};
    std::uint32_t next{},metadata{};
    std::vector<char> text;
    std::array<std::uint32_t,8> context{};
};
// 0은 null이며 map의 주소는 삽입 후에도 안정된다. nextToken=0은 자동 토큰 발급 소진 상태다.
// 체인의 head는 메뉴 항목의 styled 필드 등 실제 소유 필드에 보관하고 이 표에서는 중복 소유하지 않는다.
struct StyleTextState {
    std::map<std::uint32_t,StyleTextFragment> fragments;
    std::uint32_t nextToken{1};
};
// 문자열/그림이 같은 자동 토큰과 안정된 조각 표를 사용하도록 등록한다. null 토큰과 덮어쓰기를 피한다.
std::uint32_t RegisterStyleTextFragment(StyleTextState& state,StyleTextFragment fragment);
// 문자열 경계는 입력 text/context를 변경하지 않는다. 파생 경계는 현재 head/next를 바꿀 수 있다.
// metrics는 캐시 높이/상승/하강이고 measure는 signed 폭/높이다. release는 scalar 반납 직전 통지다.
struct StyleTextHooks {
    std::function<std::array<std::int32_t,3>(const std::array<std::uint32_t,8>&)> textMetrics;
    std::function<void(std::span<const char>,const std::array<std::uint32_t,8>&,std::int32_t&,std::int32_t&)> textMeasure;
    std::function<void(std::span<const char>,ScreenPoint,const std::array<std::uint32_t,8>&)> textDraw;
    std::function<void(std::uint32_t)> release;
    std::function<std::array<std::int32_t,2>(std::uint32_t)> externalMetrics,externalMeasure;
    std::function<void(std::uint32_t,ScreenPoint,const std::array<std::uint32_t,8>*)> externalDraw;
    std::function<void(std::uint32_t,std::uint8_t)> externalDestroy;
    std::function<std::uint32_t(char)> color;
    // ~[...] 내부의 그림/옵션 처리는 명시 경계다. 필요한 문맥 변경과 조각 추가를 같은 head에 수행한다.
    std::function<void(std::string_view,std::array<std::uint32_t,8>&,std::uint32_t&)> bracket;
};
// 같은 메뉴/텍스트 Gump에 head를 보관하고 조각의 소유권을 전달하는 원본 처리기다.
// 자동 토큰은 호스트용 식별자이며 PE 메모리 주소나 Gump 등록 ID로 재해석하지 않는다.
class StyleText {
public:
    // 상태/글꼴/출력 표면/팔레트는 처리기와 연결 훅보다 오래 살아야 한다.
    StyleText(StyleTextState& state,StyleTextHooks hooks,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 004b93c0/CD 004b4de0: 명시 길이를 복사하고 NUL을 붙인다. -1 비트는 범위 안 NUL 탐색이다.
    std::uint32_t NewText(std::span<const char> text,const std::array<std::uint32_t,8>& context,
        std::uint32_t length=0xffffffffU,std::uint32_t metadata=0) const;
    // 004bbda0/CD 004b7dc0: 원본처럼 +8 DWORD를 복사 길이로 사용하고 새 +8은 0으로 쓴다.
    // 기본 파서 조각의 +8=0이면 복제 문자열은 빈 문자열이다. 호스트 범위 밖 길이는 거부한다.
    std::uint32_t CloneText(std::uint32_t token) const;
    // 004756e0/CD 004505c0: 현재 꼬리에 단일 조각을 붙인 다음 새 조각 next를 0으로 쓴다.
    void Append(std::uint32_t& head,std::uint32_t fragment) const;
    // 00475720/CD 00450600: source 체인을 그대로 이전하고 source를 0으로 만든다.
    void Transfer(std::uint32_t& head,std::uint32_t& source) const;
    // 004756b0/CD 00450590: 다음 토큰을 삭제 전에 저장하고 scalar 삭제를 반복한 뒤 현재 head를 0으로 쓴다.
    void Clear(std::uint32_t& head) const;
    // 텍스트 소멸은 문자열/next를 비운다. flags&1일 때 release 경계 후 map에서 반납한다.
    // 파생 조각은 externalDestroy가 자원을 해제하며 scalar map 반납은 같은 순서를 따른다.
    void Destroy(std::uint32_t token,std::uint8_t flags) const;
    // 문자열은 캐시 높이/상승을 반환한다. 그림 등 파생 조각은 가상 +4 경계를 공급한다.
    std::array<std::int32_t,2> Baseline(std::uint32_t token) const;
    // 문자열은 저장 문맥/NUL 문자열을 측정한다. 파생 조각은 가상 +8 경계를 공급한다.
    std::array<std::int32_t,2> MeasureFragment(std::uint32_t token) const;
    // 가상 +c: override가 있어도 font는 조각의 저장 슬롯을 유지한다.
    void DrawFragment(std::uint32_t token,ScreenPoint position,const std::array<std::uint32_t,8>* overrideContext=nullptr) const;
    // 00475760/CD 00450640: 첫 순회에서 최대 상승을 구하고 현재 head부터 두 번째 순회를 시작한다.
    // 출력 참조가 같아도 높이→폭 쓰기와 DWORD 감김, 판본별 head/next 조회 시점을 유지한다.
    void Measure(const std::uint32_t& head,std::int32_t& width,std::int32_t& height) const;
    // 004755e0/CD 004504c0: 시작 head를 저장한다. 최대 상승으로 y를 맞추고 측정 폭으로 x를 이동한다.
    // 첫 순회의 최대 캐시 높이를 반환한다. override는 배치 측정 문맥을 바꾸지 않는다.
    std::int32_t Draw(std::uint32_t head,ScreenPoint position,const std::array<std::uint32_t,8>* overrideContext=nullptr) const;
    // 004bb430/CD 004b7770: ~ 색/슬롯/스타일·그림자/초기 문맥 복귀·리터럴 ~~를 해석해 기존 head 뒤에 붙인다.
    // 닫는 ]가 있는 대괄호 명령은 bracket 경계가 필요하다. 결과의 마지막 빈 조각도 유지한다.
    void Parse(std::uint32_t& head,std::string_view text,const std::array<std::uint32_t,8>& context) const;
private:
    // 실제 도달한 토큰의 안정된 저장소를 읽고 null/미등록 토큰을 거부한다.
    StyleTextFragment& Node(std::uint32_t token) const;
    // 현재 next를 순회해 꼬리를 찾는다. 잘못된 호스트 순환 체인은 예외로 거부한다.
    std::uint32_t Tail(std::uint32_t head) const;
    // 원본 색 전역을 읽는 경계는 실제 색 명령을 만났을 때만 요구한다.
    std::uint32_t Color(char command) const;
    StyleTextState& state_;
    StyleTextHooks hooks_;
    o::OriginalEdition edition_;
};
// 기존 FontStore의 캐시 메트릭·첫 폭 표 측정·실제 글리프 출력을 문자열 조각에 공급한다.
// draw가 null이면 출력 경계를 보존하고 측정/메트릭만 연결한다.
void BindStyleTextFonts(StyleTextHooks& hooks,const GumpText& text,const GumpTextDraw* draw=nullptr);
// 실제 원본 색 표를 짧은 명령 색 조회에 연결한다. 표의 수명은 훅보다 길어야 한다.
void BindStyleTextPalette(StyleTextHooks& hooks,const PaletteColorTable& palette);
// 같은 메뉴 항목의 styled head를 Clear/Parse/두 번 측정에 연결한다. lookup과 처리기는 안정되어야 한다.
struct MenuGumpLayoutHooks;
void BindMenuLayoutStyleText(MenuGumpLayoutHooks& hooks,const StyleText& text);
}
