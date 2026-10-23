// 원본 Htmlgump.cpp·Ubergump.cpp의 조건/메뉴 명령과 본문을 구성한다.
#pragma once
#include "o/ConfigInterface.h"
#include "client/HtmlTextParser.h"
#include <functional>

namespace netstorm::client {
// 다이얼로그 항목이 실행할 명령과 인자이다.
struct DialogAction { /* command: 선택·클릭할 때 실행할 명령 이름 또는 명령 ID. */ /* argument: 타입·패턴·명령에 별도로 전달하는 첫 인자. */ std::string command, argument; };
// 다이얼로그의 표시 문구·동작·버튼 상태이다.
struct DialogItem {
    // label: 화면에 표시할 항목 문구 또는 패턴 번호.
    std::string label;
    // action: 항목의 활성화 때 실행할 명령·인자 자료.
    DialogAction action;
    // menu: 메뉴 여부.
    // checked: 선택·체크 표식을 표시할지.
    // enabled: 해당 동작을 입력·실행할 수 있는지.
    // instant: 버튼 해제 대기 없이 누름 즉시 동작할지.
    bool menu{}, checked{}, enabled{true}, instant{};
    std::uint32_t menuColumn{}; // Tell의 현재 세로 열 번호다. 하단 버튼은 사용하지 않는다.
    bool checkedDirective{}; // 체크값이0인 $Checked도 일반 $Menu sentinel 분기와 구별한다.
};
// 다이얼로그 본문 한 문단과 글꼴 효과이다.
struct DialogParagraph { /* text: 표시하거나 해석할 본문 문자열. */ std::string text; /* heading: 이동 방향 번호. */ int heading{}; /* italic: 본문에 기울임 글꼴 효과를 적용할지. */ /* bold: 본문에 굵은 글씨 효과를 적용할지. */ bool italic{}, bold{}; };
// 다이얼로그 한 페이지의 문단·항목·배경·시간 제한 동작이다.
struct DialogPage {
    std::vector<std::string> commands; // HTML의 <$...> 원시 명령을 나타난 순서대로 실제 입력 큐에 예약한다.
    std::string htmlBody; // 실제경로는1차HTML→메타제거→LF를<br>로재구성한2차HTML입력이다. 검사호환경로는기존원문본문이다.
    bool hasContent{}; // 원본004c8d50처럼1차HTML출력에ASCII alnum이있어대화상자가필요한지나타낸다.
    // 기존 검사/관찰용 Warning 인자다. 실행은 commands의 원래 순서와 실제 입력 큐를 사용한다.
    std::vector<std::string> warnings;
    // paragraphs: 표시 순서대로 해석한 다이얼로그 문단 목록.
    std::vector<DialogParagraph> paragraphs;
    // items: 읽거나 생성한 항목의 순서 있는 목록.
    std::vector<DialogItem> items;
    std::uint32_t menuColumns{}; // 원본 Tell+21c8의 현재 메뉴 열 수다. 빈 맵 목록도 생성한 열을 보존한다.
    std::vector<std::uint32_t> menuColumnFlags; // 실제 표시 어댑터가 소유하는 열 생성 자료다. 각 Begin의 플래그를 보존한다.
    // background: 배경 출력 자료 또는 배경 Gump 토큰.
    std::string background;
    // timeoutSeconds: 다이얼로그 시간 제한의 길이(초).
    int timeoutSeconds{};
    // timeout: 다이얼로그 시간 제한에 도달하면 실행할 동작.
    // onExit: 페이지에서 나갈 때 실행할 동작.
    DialogAction timeout, onExit;
};
// 메타 추출 시점의 명령 조회/문자열 수명과 실제 메뉴 목록 장치를 공급한다.
struct DialogMetadataHooks {
    std::function<void(const DialogAction&)> resolveAction; // 항목 종류 처리 전에 원본 명령 조회와 인자 변환 효과를 수행한다.
    std::function<void(DialogPage&,DialogItem)> appendMenu; // Menu/Checked가 선택된 바로 그 시점에 현재 열과 실제 목록을 추가한다.
    std::function<void(std::string_view,std::string_view)> resolveOnExit; // OnExit만Trim전regex캡처명령/인자를즉시해석한다.인자뷰는호출중에만유효하다.
};
// 기존독립검사의문자열호환필터다. 실제Tell은아래현재상태/Config효과를받는overload만사용한다.
std::string FilterDialogConditions(std::string_view source);
// 기존독립검사의원문/메타추출호환경로다. 실제게임의HTML효과·Config조회시점검증으로세지않는다.
DialogPage ParseDialogPage(std::string_view source);
// 원본004cf270의1차HTML→기존메타추출→LF를<br>로재구성한다. source는선행전체Config치환결과다.
// 같은state/globals/hooks로DialogBodyLayout::SetBody를호출해2차HTML을수행해야한다. inline명령은선택시실제hook을즉시호출한다.
DialogPage ParseDialogPage(std::string_view source,HtmlTextParserState& state,HtmlTextParserGlobals& globals,HtmlTextParserHooks hooks,
    o::OriginalEdition edition=o::OriginalEdition::Patch1078,DialogMetadataHooks metadata={});
}
