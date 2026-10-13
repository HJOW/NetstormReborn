// 기본 Gump의 우클릭 도움말 메뉴 생성 요청을 같은 노드/루트 저장소에 연결한다.
#pragma once
#include "client/ButtonGump.h"
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 도움말 항목 본문/이벤트 전역과 %sHelp로 만든 임시 주제 문자열이다.
// 패치: 0054db38/00542754/00565df0. CD: 0052e8cc/005128dc/0056b6f8.
struct GumpHelpState {
    // body: 본문 문자열 또는 본문을 담당하는 Gump 토큰.
    // event: 대상 처리기에 전달할 이벤트 ID.
    std::uint32_t body{},event{};
    // topic: 도움말에서 조회하거나 요청할 주제 ID.
    std::string topic;
};
// 원본 제어 메뉴의 시작·제목·항목·부착과 사운드를 공급하는 경계다.
// entry의 마지막 인자는 패치판 0, CD판 없음이다. 각 콜백은 같은 노드/전역/현재 루트를 바꿀 수 있다.
// text는 원본 문자열 토큰의 NUL 종료 바이트 문자열을 반환한다. 유효 문자열/포인터만 허용한다.
struct GumpHelpHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> node;
    // text: 요청한 본문을 실제 텍스트 처리기에 전달하는 콜백.
    std::function<std::string(std::uint32_t)> text;
    // beginMenu: 도움말 메뉴 구성을 시작하는 콜백.
    std::function<void(std::uint32_t)> beginMenu;
    // title: 메뉴 제목을 설정하는 콜백.
    std::function<void(std::uint32_t)> title;
    // entry: 표시 문구와 명령 인자로 메뉴 항목을 추가하는 콜백.
    std::function<void(std::uint32_t,std::uint32_t,const std::string&,std::uint32_t,std::uint32_t,
        std::optional<std::uint32_t>)> entry;
    // attachMenu: 생성한 도움말 메뉴를 지정 위치에 부착하는 콜백.
    std::function<void(std::int32_t,std::int32_t,std::uint32_t)> attachMenu;
    // sound: 해당 동작의 소리를 재생하는 콜백.
    std::function<void(std::string_view)> sound;
};
// 00464990/CD 00493720의 조건·문자열 조립·현재 상태 재조회·호출 순서를 복원한다.
// 실제 MenuGump/Form/폰트·사운드 장치의 몸체는 해당 경계에서 별도로 공급한다.
class GumpHelp {
public:
    // 생성 표/전역/노드/문자열 저장소는 도움말 처리기보다 오래 살아야 한다. node는 필수다.
    GumpHelp(GumpCreateState& create,GumpHelpState& state,GumpHelpHooks hooks);
    // 오른쪽 누름이고 현재 alternateTextPointer(+7c)가 있을 때만 메뉴를 열고 1을 반환한다.
    // 제목은 beginMenu 후 현재 textPointer(+78), 없으면 현재 alternateTextPointer를 사용한다.
    // 좌표 DWORD의 signed 비트를 유지하고 항목 생성 후의 현재 entries[0]을 부모로 넘긴다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
private:
    // 같은 기본/파생 노드의 현재 도움말 문자열 필드를 조회한다.
    GumpCreateNode& Node(std::uint32_t token) const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // state_: 도움말 항목 본문/이벤트 전역과 %sHelp로 만든 임시 주제 문자열이다 (GumpHelpState).
    GumpHelpState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (GumpHelpHooks).
    GumpHelpHooks hooks_;
};
// 기본 도움말을 ButtonGump의 fallback으로 연결한다. help는 해당 훅보다 오래 살아야 한다.
void BindGumpHelp(ButtonGumpHooks& buttons,const GumpHelp& help);
}
