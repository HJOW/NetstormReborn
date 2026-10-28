// 원본 Save as의 두 버튼 부모와 이름 확정을 기존 문자/Gump/파일 장치에 연결한다.
#pragma once
#include "client/GumpDecoration.h"
#include "client/RawTextEntryContext.h"
#include <optional>

namespace netstorm::client { // 실제 이름 대화창은 공통 등록표와 편집 작업본을 빌린다.
class EditorFiles; // 쓰기 가능한 편집 namespace의 실제 경로/존재 장치다.
// VT00517bc8의 장식 뒤 제목/두 버튼/편집/두 모드 필드다.
struct SaveFortNameDialogNode: GumpDecorationNode {
    std::uint32_t title{},ok{},cancel{},entry{}; // +c4/+c8/+cc/+d0의 실제 자식 토큰이다.
    std::uint32_t fortMode{},canCancel{}; // +d4/+d8의 전체 원문 DWORD를 보존한다.
};
// 이름 확정 본체의 선택된 실제 효과다. 경로 존재는 원문 GetFileAttributesA와 같은 비영 의미다.
struct SaveFortNameConfirmHooks {
    std::function<void(std::span<char>)> copyText; // 실제 현재 TextEntry.CopyText이며 원문 지역256BYTE에 쓴다.
    std::function<int(std::int32_t)> alnum; // 원본 CRT isalnum의 signed char 인자를 전달한다.
    std::function<std::string(std::string_view)> path; // fortSpec 선택과 현재 이름 PathSpec이다.
    std::function<std::int32_t(std::string_view)> exists; // 디렉터리도 비영인 실제 작업본 속성 조회다.
    std::function<std::span<std::uint8_t>()> header; // 매번 현재 owner의 같은20BYTE Template 헤더다.
    std::function<void(std::string_view)> setName,tell,readFort; // 전체 이름 부모·NameRequired·일반 입력 큐 예약이다.
};
// 원문004cf9a0의 전체 정상 반환이며 앞선 부분 효과를 실패 때 되돌리지 않는다.
std::uint32_t ConfirmSaveFortName(const std::uint32_t& fortMode,const SaveFortNameConfirmHooks& hooks);
// 원문004cfab0은 canCancel0이면 안내 후0이고 비영이면1이다.
std::uint32_t CancelSaveFortName(const std::uint32_t& canCancel,const std::function<void(std::string_view)>& tell);
// 실제 Client가 제공하는 현재 장치이며 매 창의 callback은 해당 모달보다 오래 살아야 한다.
struct SaveFortNameDialogContext {
    std::function<std::uint32_t(std::uint32_t,std::string_view)> sendMoney; // 원본+d4가 송금 대상인 숫자 창은 파일 장치 대신 같은 확정을 사용한다.
    const EditorFiles* work{}; // 보호 원본과 구별된 실제 namespace를 반드시 지정한다.
    RawTextEntryContext editor; // 같은 공통 문자 UI의 벽시계/clipboard 장치다.
    std::function<std::string(std::string_view)> translate,fortPath; // 현재 번역과 fortSpec 선택/PathSpec의 원문 바이트 문자열이다.
    std::function<std::span<std::uint8_t>()> header; // 현재 Template owner의 header를 빌린다.
    std::function<void(std::string_view)> setName,tell,readFort; // 전체 이름 부모에는 같은 work 장치를 전달해야 한다.
    std::string currentName; // 생성 시 원문 current 이름을 안정된 NUL 문자열로 보유한다.
};
// 생성/입력 부모는 실제 공통 장치 호출만 조립하며 기본 Gump를 별도 구현하지 않는다.
struct SaveFortNameDialogHooks {
    std::function<SaveFortNameDialogNode*(std::uint32_t)> node; // 같은 실제0xdc 저장소를 조회한다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 현재 자식의 공통 실제 필드다.
    std::function<TextEntryGumpNode*(std::uint32_t)> entry; // 현재 편집 자식의 추가 필드다.
    std::function<std::uint32_t(std::size_t)> allocate; // 실제 root/편집/배경/제목/돌 버튼 저장소다.
    std::function<void(std::uint32_t,float,float)> decoration; // 기존00465d10 전체 생성이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::string_view)> textEntry; // 기존00452430이며 font/background0을 보존한다.
    std::function<void(std::uint32_t,std::string_view)> excluded; // 기존00451b80 소유 문자열 교체다.
    std::function<void(std::uint32_t,float,float)> background; // 원문 테두리 flags/type/frame의 실제 생성이다.
    std::function<void(std::uint32_t,std::string_view)> title,stone; // font20 제목과70×18 돌 버튼의 실제 생성이다.
    std::function<void(std::uint32_t)> fitTitle,fitStone,push; // 원문 같은 가상 fit/push 순서다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 현재 실제 파생의 가상+20이다.
    std::function<void(std::uint32_t,std::int32_t)> depth; //004652b0의 같은 signed 깊이 조정이다.
    std::function<void(std::uint32_t)> modal,kill; // 원문00466600과 지연 Kill이다.
    std::function<std::uint32_t(std::uint32_t)> accept,cancel; // 현재 창의 두 원문 가상 확정이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle; //00464990 기본 Help 위임이다.
    std::function<std::string(std::string_view)> translate; // 현재 언어 번역이며 생성 순서대로 읽는다.
};
// 원본004d3250/004d3490과00487460을 같은 저장소에 실행한다.
class SaveFortNameDialog {
public:
    SaveFortNameDialog(GumpCreateState& state,SaveFortNameDialogHooks hooks); // 공통 등록표와 실제 장치를 빌린다.
    std::uint32_t Show(std::uint32_t fortMode,std::uint32_t canCancel,std::string_view current,std::optional<std::string_view> initial={}) const; // 실제 모달 root 토큰이다.
    std::uint32_t ShowMoney(std::uint32_t target) const; //004d3860/004d3b60의 숫자 전용 형제 창이며 공통 장식/두 버튼을 재사용한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; // Enter/두 버튼 외에는 기본 부모에 위임한다.
private:
    SaveFortNameDialogNode& Node(std::uint32_t token) const; // 같은 실제 파생 저장소를 요구한다.
    GumpCreateNode& Base(std::uint32_t token) const; // 선택된 자식의 같은 현재 필드다.
    void Buttons(std::uint32_t token,float y) const; //00487530/00487360의 생성·Fit·배치다.
    GumpCreateState& state_; // 화면 크기/실제 공통 루트의 현재 저장소다.
    SaveFortNameDialogHooks hooks_; // 원문 하위와 확정 효과를 실행한다.
};
}
