// 전용 Help의 전체 자식 생성·배치·본문/그림 갱신을 기존 실제 Gump 처리기에 조립한다.
#pragma once
#include "client/QuickHelpControl.h"
#include "client/HtmlBodyGump.h"
#include <optional>
namespace netstorm::client {
// 0xd0 창의 남은 필드다. 기존 제어/이력 입력도 같은 QuickHelpWindowNode 기본 주소를 빌린다.
struct QuickHelpContentNode: QuickHelpWindowNode {
    std::uint32_t picture{},decoration{},information{},body{},stream{}; // +a0/+a4/+a8/+ac/+bc다.
    std::uint32_t innerBackground{},pictureBackground{},bodyBackground{}; // +c0/+c4/+c8의 실제 배경 자식이다.
};
// 실제 원본 전역은 창의 수명과 별개다. 캐시/마지막 popup은 다른 Help 요청에도 유지한다.
struct QuickHelpWindowState {
    std::uint32_t id{}; // 005ca990의 현재 등록 serial이다.
    std::optional<std::string> helpData; // 005ca994의 null/로드한 본문을 구별한다. 성공 시 마지막 LF를 포함한다.
    std::int8_t kind{19}; // 0054042e의 실제 전용 창 종류 BYTE다.
    std::uint32_t color{},buttonUnused{},scriptCommand{0x02000095}; // 5b5dc8/5acd24/542948의 현재 색/돌 버튼 unused/스크립트 복귀 명령이다.
    std::string scriptName; // 005cab40의 현재 스크립트 이름이다. 처음 NUL까지만 사용한다.
    bool battle{},challenge{}; // 00594fbc/fc8의 현재 기본 도움말 선택 모드다. battle이 우선이다.
    std::uint32_t infoMode{}; // 00565ec4의 현재 다중 줄 정보 해석 모드다.
    std::uint32_t noPictureA{},noPictureB{}; // 0054120c/12dc의 실제 그림 제외 TYPE를 공급한다.
};
// 모든 생성/표시 자식은 기존 실제 클래스다. 이 계층은 별도 간략 창/글자 렌더러를 만들지 않는다.
struct QuickHelpWindowHooks {
    std::function<QuickHelpContentNode*(std::uint32_t)> node; // 같은 전용 파생 저장소다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 현재 부모/자식의 공통 기본 필드다.
    std::function<HtmlBodyGumpNode*(std::uint32_t)> html; // 같은 두8192바이트 영역이다.
    std::function<void(std::uint32_t,std::uint32_t,float,float,std::uint32_t)> constructBase; // 실제 기본 Gump 생성이다.
    std::function<std::uint32_t(std::size_t)> allocate; // 각 원본 크기에 맞는 실제 파생 저장소 토큰이다.
    std::function<void(std::uint32_t,float,float)> constructDecoration; // 실제 GumpDecoration(0,0)이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> constructBackground; // 실제 배경 생성의 여섯 인자다.
    std::function<void(std::uint32_t)> constructHtml,fitButton; // 실제 HTML 부모 생성/돌 버튼 글자 맞춤이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::string_view,std::uint32_t,std::uint32_t)> constructButton; // 실제 돌 버튼의 원래 다섯 인자다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t,std::uint32_t)> constructInformation; // 실제 다중 줄 생성이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> constructPicture; // 실제 ImageGump(type,frame,8)이다.
    std::function<std::string(std::string_view)> translate,config,substitute; // 번역/파일 키/8192바이트 본문 치환이다.
    std::function<std::optional<std::string>(std::string_view)> load; // 실제 VFS 파일의 바이트다. 실패와 빈 성공 파일을 구별한다.
    std::function<void(std::string_view)> loadError; // 원본 등급9 로드 실패 진단이다.
    std::function<std::uint32_t(std::int32_t,std::int32_t,std::int32_t)> nearestColor; // 현재 팔레트의 RGB(55,51,54) 조회다.
    std::function<bool()> scripted; // 현재 스크립트 양의 깊이 판정이다.
    std::function<void(std::uint32_t)> kill,push,clearBody; // 실제 지연 삭제/가상 분리/HTML 표시 자식 정리다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach,baseAttach; // 자식 가상 부착과 자신 기본 부착이다.
    std::function<std::int32_t(std::uint32_t)> lines; // 현재 정보 pane 버퍼 +4의 완료 줄 수다.
    std::function<void(std::uint32_t,std::uint32_t,std::string_view,std::uint32_t,std::uint32_t)> render; // 실제 전용 HTML→같은 body 배열/링크와 information/stream이다.
    std::function<std::optional<std::int32_t>(std::uint32_t)> helpFrame; // 선택한 TYPE의 현재 +120을 치환 뒤 읽는다. TYPE 없음은 null이다.
    std::function<void(std::uint32_t,std::string_view)> bodyText; // 같은 HTML 부모의 실제 다중 줄 표시 갱신이다.
};
// 실제4c5380/58a0/5970/5d60을 전체 몸체로 옮긴다. Client 가상 분배 연결은 호출자 책임이다.
class QuickHelpWindow {
public:
    QuickHelpWindow(QuickHelpWindowState& state,QuickHelpPopupState& popup,QuickHelpWindowHooks hooks); // 같은 캐시/현재 표식과 실제 트리를 빌린다.
    std::uint32_t Construct(std::uint32_t token,std::uint32_t learnType) const; // 전용 자식/라벨/파일 캐시·450×350·등록 serial을 설치한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 그림/선택 버튼/본문/정보의 현재 크기를 원본 순서로 배치한다.
    void Reset(std::uint32_t token) const; // 그림/본문 자식/이전 정보 pane을 정리하고 새 정보 pane을 만든다.
    void Show(std::uint32_t token,std::uint32_t type,std::string_view topic) const; // 기본 주제/파생 HTML·치환·그림·실제 본문을 갱신한다.
private:
    QuickHelpContentNode& Node(std::uint32_t token) const; // 실제 전용 파생의 현재 필드를 읽는다.
    GumpCreateNode& Base(std::uint32_t token) const; // 같은 현재 기본/자식을 읽는다.
    HtmlBodyGumpNode& Body(std::uint32_t token) const; // 실제 body 토큰의 소유 배열을 읽는다.
    std::uint32_t Allocate(std::size_t bytes) const; // 원본 new 실패는 안전한 호스트에서 예외로 드러낸다.
    void Width(std::uint32_t token,double value) const; // 같은 x87 계산값으로 기본/배율 폭을 각각 저장한다.
    void Height(std::uint32_t token,double value) const; // 같은 x87 계산값으로 기본/배율 높이를 각각 저장한다.
    QuickHelpWindowState& state_; // 실제 Help 전역/파일 캐시다.
    QuickHelpPopupState& popup_; // 지식 카드/Back과 같은 마지막 popup 저장소다.
    QuickHelpWindowHooks hooks_; // 실제 기존 GUI/Config/VFS 효과다.
};
}
