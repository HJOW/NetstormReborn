#include "client/MessageGump.h"
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kMessageVtable=0x00517b60U; // 보호10.78의 메시지 파생 가상 표다.
constexpr std::int32_t kWarning=17; //원문Script Warning의실제메시지종류다.
constexpr std::int32_t kGameTime=16; //원문게임시간메시지의실제종류다.
constexpr std::int32_t kPause=11; // 원문 분배기와 생성자가 사용하는 실제 Pause 종류다.
constexpr std::string_view kGameTimeText="~E~yGame Time~r:~w %d:%02d:%02d"; //PE00516024의원문번역서식이다.
constexpr std::string_view kPauseText="~E~wGame Paused!"; // PE00516010의 현재 원문 번역 키다.
}
// 한문자열서식의폭/정밀도/리터럴%만실제CRT로처리한다.원문에미공급된추가인자는합성하지않는다.
std::string FormatWarningMessage(std::string_view pattern,std::optional<std::string_view> source) {
    pattern=pattern.substr(0,pattern.find('\0')); //원문서식도첫NUL까지만해석한다.
    const std::string text=source ? std::string(source->substr(0,source->find('\0'))) : std::string("(null)"); //원문VS2003의NULL%s결과를호스트CRT의서로다른NULL정책과독립적으로보존한다.
    std::string result;bool consumed{}; //실제경계의유일한%s인자만한번소비한다.
    for (std::size_t index=0;index<pattern.size();) { //문자와서식토큰을원문순서대로소비한다.
        if (pattern[index]!='%') { result+=pattern[index++]; }
        else if (index+1<pattern.size() && pattern[index+1]=='%') { result+='%';index+=2; }
        else {
            const auto begin=index++; //실제이토큰의전체CRT서식을보관한다.
            if (index<pattern.size() && pattern[index]=='-') ++index;
            while (index<pattern.size() && pattern[index]>='0' && pattern[index]<='9') ++index; //가변폭*은미공급인자를필요로하므로허용하지않는다.
            if (index<pattern.size() && pattern[index]=='.') { ++index;while (index<pattern.size() && pattern[index]>='0' && pattern[index]<='9') ++index; } //문자열정밀도는바이트상한이며UTF-8코드포인트로바꾸지않는다.
            if (index>=pattern.size() || pattern[index]!='s' || consumed) throw std::logic_error("Warning의한문자열외원문서식미연결");
            consumed=true;const std::string format(pattern.substr(begin,++index-begin));char output[256]; //원문의지역256BYTE물리를넘는결과는스타일생성전에호스트오류로분리한다.
            const auto count=std::snprintf(output,sizeof(output),format.c_str(),text.c_str());
            if (count<0 || static_cast<std::size_t>(count)>=sizeof(output)) throw std::length_error("Warning의원문256BYTE서식범위초과");
            result.append(output,static_cast<std::size_t>(count));
        }
        if (result.size()>=256) throw std::length_error("Warning의원문256BYTE서식범위초과");
    }
    return result;
}

// 선택되지 않은 Challenge 이름 조회는 도달할 때만 요구한다.
MessageGump::MessageGump(const GumpCreateState& create,MessageGumpHooks hooks):hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078 || !hooks_.node || !hooks_.constructText || !hooks_.challenge || !hooks_.playerName || !hooks_.translate || !hooks_.style || !hooks_.fit)
        throw std::invalid_argument("Pause 메시지 실제 TextGump/이름/번역/측정 연결 누락");
}
// 별도 기본 노드를 만들지 않고 같은 실제 파생을 반환한다.
MessageGumpNode& MessageGump::Node(std::uint32_t token) const {
    auto* current=token ? hooks_.node(token) : nullptr;
    if (!current) throw std::logic_error("Pause 메시지의 현재 공유 노드 없음");
    return *current;
}
// Pause는 시계/음향/idle을 호출하지 않고 기한0의 영구 메시지를 측정한다.
std::uint32_t MessageGump::Construct(std::uint32_t token,std::int32_t kind,std::int32_t player,std::int32_t parameter,std::optional<std::string_view> source) const {
    if (kind!=kPause && kind!=kGameTime && kind!=kWarning) throw std::logic_error("다른 원본 메시지 종류의 생성 분기 미복원");
    hooks_.constructText(token);
    Node(token).vtable=kMessageVtable;
    Node(token).deadline=0.0;
    Node(token).kind=kind;
    Node(token).player=player;
    Node(token).parameter=parameter;
    if (hooks_.challenge()) {
        if (!hooks_.challengePlayerName) throw std::logic_error("Challenge 메시지의 실제42bbb0 이름 조회 미연결");
        hooks_.challengePlayerName(player);
    } else hooks_.playerName(player);
    if (Node(token).kind==kGameTime) {
        if (!hooks_.gameTimeSeconds) throw std::logic_error("게임시간메시지의현재CRT시계미연결");
        const auto seconds=hooks_.gameTimeSeconds(); //원문은이위치의signed정수초에서시/분/초를계산한다.
        const auto hours=(seconds/60)/60,minutes=(seconds/60)%60,remainder=(seconds%60)%60; //IDIV의음수절삭/나머지도보존한다.
        const auto format=hooks_.translate(kGameTimeText); //정수계산뒤현재언어번역을조회한다.
        char text[256]; //원문의현재256BYTE서식버퍼이며초과는호스트에서명시거부한다.
        const auto length=std::snprintf(text,sizeof(text),format.c_str(),hours,minutes,remainder);
        if (length<0 || static_cast<std::size_t>(length)>=sizeof(text)) throw std::length_error("게임시간메시지원문서식버퍼범위초과");
        hooks_.style(token,std::string_view(text,static_cast<std::size_t>(length)));
        if (!hooks_.wallSeconds) throw std::logic_error("게임시간메시지의현재벽시각미연결");
        const auto now=hooks_.wallSeconds();Node(token).deadline=0.1+now; //PE00501960은double0.1이며1초가아니다.
        if (!hooks_.idle) throw std::logic_error("게임시간메시지의실제Idle등록미연결");hooks_.idle(token,true);
    } else if (Node(token).kind==kWarning) {
        if (!hooks_.warningSound) throw std::logic_error("Warning메시지의실제newWarnMessage음향미연결");hooks_.warningSound();
        const auto pattern=hooks_.translate("%s"); //원문은본문이아닌고정%s서식만현재언어로번역한다.
        const auto text=FormatWarningMessage(pattern,source);hooks_.style(token,text); //음향/번역복귀뒤현재입력의첫NUL을읽는다.
        if (!hooks_.wallSeconds) throw std::logic_error("Warning메시지의현재벽시각미연결");
        const auto now=hooks_.wallSeconds();Node(token).deadline=10.0+now; //00418070은float10을현재double벽시각에더한다.
        if (!hooks_.idle) throw std::logic_error("Warning메시지의실제Idle등록미연결");hooks_.idle(token,true);
    } else { const auto text=hooks_.translate(kPauseText);hooks_.style(token,text); } //Pause는기존영구기한0/무음향/무Idle을유지한다.
    hooks_.fit(token,0,0);
    return token;
}
}
