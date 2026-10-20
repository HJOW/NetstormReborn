// 원본 Quick Help의 전용 창 재사용·이력·팝업 복귀·입력 분배를 같은 Gump 저장소에 복원한다.
#pragma once
#include "client/GumpCreate.h"
#include "client/InputEvent.h"
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client {
// 005ca938/940이다. 모든 팝업과 Help Back이 같은 마지막 명령/80바이트를 사용한다.
struct QuickHelpPopupState {
    std::uint32_t command{}; // Back의 마지막 이력이 사라질 때 다시 예약하는 명령이다.
    std::array<char,80> argument{}; // 첫NUL 뒤 바이트도 원본 표식 저장소처럼 유지한다.
    void Mark(std::uint32_t code,std::optional<std::string_view> text=std::nullopt); // null은 전체0,문자열은 NUL까지 복사한다.
    std::string_view Argument() const; // 현재 첫NUL까지 읽는다. NUL 없는 저장소는 안전한 호스트에서 거부한다.
};
// 원본4c52c0/4c6d00의 단일 연결 이력을 소유한다. 중간 중복은 유지하고 머리의 연속 중복만 생략한다.
class QuickHelpHistory {
public:
    void Push(std::string_view topic); // 첫NUL/ASCII 대소문자 비교 뒤 원래 바이트의 주제를 저장한다.
    std::optional<std::string> Back(); // 현재 항목 하나를 제거하고 새 머리를 반환한다. 빈 주제와 이력 없음은 다르다.
    std::span<const std::string> Topics() const; // 저장 순서는 가장 오래된 항목부터이며 표시/검사에서 읽기만 한다.
private:
    std::vector<std::string> topics_; // 문자열은 원래 창 수명과 독립인 전역 이력의 소유 바이트다.
};
// 004c62a0이 매번 조회하는 등록 ID/화면 영역/실제 루트다.
struct QuickHelpOpenState {
    std::uint32_t helpId{},alternateId{},root{}; // Help/대체 창 등록 ID와 현재 UI 루트 토큰이다.
    std::int32_t areaLeft{},areaWidth{}; // 원본 signed32 화면 영역이며 최종 부착에서 float으로 저장한다.
};
// 실제 전용 파생 생성자·공통 Gump 모달/트리만 공급한다. 일반 Tell 생성자로 대체하지 않는다.
struct QuickHelpOpenHooks {
    std::function<void()> closePopups; // 00466710의 현재 sticky 팝업 Kill이다.
    std::function<std::uint32_t(std::uint32_t)> lookup; // 00464020의 현재 등록 ID 조회다.
    std::function<bool()> noModal; // 004c8e90의 현재 모달 없음 판정이다.
    std::function<std::uint32_t()> currentModal; // noModal 이후의 현재 모달을 읽는다.
    std::function<std::uint32_t(std::uint32_t)> construct; // new0xd0/004c5380을 통해 실제 전용 창을 만든다.
    std::function<GumpCreateNode*(std::uint32_t)> node; // 실제 기본 필드이며 별도 복사본을 만들지 않는다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 같은 창의 가상+20이다.
    std::function<void(std::uint32_t,std::uint32_t)> addDepth; // 004652b0처럼 자식을 포함해 delta를 더한다.
    std::function<void(std::uint32_t,std::uint32_t)> modal; // 같은 공통 Gump의 모달 진입이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> handle; // 같은 파생 창의 가상+28이다.
};
// topic은 호스트 문자열 토큰이다. 다른 모달에 막혀도 이미 존재한 Help 토큰은 반환한다.
std::uint32_t OpenQuickHelpWindow(const QuickHelpOpenState& state,const QuickHelpOpenHooks& hooks,
    std::uint32_t topic,std::int32_t mode,std::uint32_t learnType);

// 원본0xd0 파생의 입력에 필요한 실제 기본 필드/버튼/학습 TYPE다. 전용 생성/본문은 별도 연결 대상이다.
struct QuickHelpWindowNode: GumpCreateNode {
    std::uint32_t cancelButton{},learnButton{},backButton{}; // +b0/+b4/+b8 자식 토큰이며 비교 값은 그 자식의 +2c serial이다.
    std::uint32_t learnType{}; // +cc 학습 요청 TYPE다. 0이면 학습 없이 닫는다.
};
// 실제 전용 본문/링크/스크롤과 현재 공통 Gump 경계를 공급한다. 도달한 경계만 필수다.
struct QuickHelpEventHooks {
    std::function<std::array<float,2>(InputEvent)> local; // 실제004650a0은 모든 사건에서 가장 먼저 호출된다.
    std::function<GumpCreateNode*(std::uint32_t)> node; // 현재 버튼의 serial을 조회한다.
    std::function<std::string(std::uint32_t)> typeName; // 실제49a840의 TYPE 이름이다. signed 음수 비트도 그대로 전달한다.
    std::function<std::string(std::string_view)> translate; // 실제4de9a0으로 "Type" 접미를 번역한다.
    std::function<std::string(std::uint32_t)> text; // 유효한 현재 호스트 문자열 토큰의 첫NUL 본문이다.
    std::function<void(std::uint32_t,std::string_view)> request; // 같은 창의 QuickHelpRequest::Open이다.
    std::function<float()> pageHeight; // 현재 스크롤 본문 버퍼의 +40 float이다.
    std::function<void(std::int32_t)> scroll; // 현재 본문의 실제004ba380이다.
    std::function<void(std::string_view)> link; // 실제4a00c0 주제/프로토콜 분배다.
    std::function<void()> kill; // 실제00466480의 지연 삭제다. 공용 popup 상태를 바꿀 수 있다.
    std::function<void(std::uint32_t)> learn; // 실제00492a30의 기술 학습 요청이다.
    std::function<void(std::uint32_t,std::string_view,std::string_view)> queue; // 실제00452f90에 같은 현재 문자열을 두 번 전달한다.
    std::function<std::uint32_t(InputEvent)> base; // 실제 기본 GumpHelp00464990의 현재 입력이다.
};
// 004c6e00/004c70a7의 전체 입력 분배다. 본문/전용 창 생성·Client 명령 소비자는 후속 연결이 필요하다.
std::uint32_t DispatchQuickHelpWindow(QuickHelpWindowNode& window,InputEvent event,QuickHelpHistory& history,
    QuickHelpPopupState& popup,const QuickHelpEventHooks& hooks);
}
