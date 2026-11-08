#include "client/MessageGump.h"
#include <algorithm> // 이름 주소의 현재 첫 NUL을 실제 필드 범위 안에서 찾는다.
#include <array> // 원문64색 서식의 두 인덱스가 같은8색 표를 사용한다.
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kMessageVtable=0x00517b60U; // 보호10.78의 메시지 파생 가상 표다.
constexpr std::int32_t kWarning=17; //원문Script Warning의실제메시지종류다.
constexpr std::int32_t kGameTime=16; //원문게임시간메시지의실제종류다.
constexpr std::int32_t kPause=11; // 원문 분배기와 생성자가 사용하는 실제 Pause 종류다.
constexpr std::int32_t kAllied=7,kStarted=9; //0048f880의 동맹 안내와004b4a20의 실제 게임 시작 안내다.
constexpr std::int32_t kCompleted=10; //4b4b40의실제기한중Leave후속이같은MessageBoard에넣는Game completed안내다.
constexpr std::int32_t kDrawRequested=6,kAllianceBroken=8; //4900e0/48f8f0이 같은 MessageBoard에 넣는 합의/해제 안내다.
constexpr std::int32_t kServerInactive=1,kPlayerInactive=2; //004b9010의 실제 응답 지연 종류이며 새 활동 수신에서 같은 키로 지운다.
constexpr std::int32_t kMoneyReceived=4,kResourceInjection=18; //004d56f0의 송금/일반 주기 지급 안내 종류다.
constexpr std::int32_t kPresent=3,kDisconnected=5; //0042b580/42bc10과 일반 접속 수신의 현재 참가/이탈 안내 종류다.
constexpr std::array<char,8> kInactiveColors{'b','r','w','u','m','y','l','o'}; // PE004d08d6..4d09f2의 지연 서식은4번 색이u다.
constexpr std::array<char,8> kAlliedColors{'b','r','w','g','m','y','l','o'}; // PE004d0f94..1db7의64서식에서 확인한 행/열 색이다.
constexpr std::string_view kGameTimeText="~E~yGame Time~r:~w %d:%02d:%02d"; //PE00516024의원문번역서식이다.
constexpr std::string_view kPauseText="~E~wGame Paused!"; // PE00516010의 현재 원문 번역 키다.
// 원문 이름 포인터의 현재 첫 NUL까지만 읽고 호스트 필드 밖의 접근은 거부한다.
std::string CurrentMessageName(std::span<const char> bytes) { const auto end=std::find(bytes.begin(),bytes.end(),'\0');if (end==bytes.end()) throw std::out_of_range("메시지 실제 이름 NUL 범위");return {bytes.begin(),end}; }
// 두 실제 문자열 인자를 같은 순서로 CRT에 전달하고 원문 지역256BYTE 밖 쓰기를 거부한다.
std::string AlliedMessage(std::string_view format,std::span<const char> first,std::span<const char> second) {
    std::size_t strings{};for (std::size_t index=0;index<format.size();++index) if (format[index]=='%') { // 실제 두 문자열 인자 외의 미공급 CRT 인자를 요구하는 번역 서식은 거부한다.
        if (++index>=format.size()) throw std::invalid_argument("동맹 안내의 잘린 서식");if (format[index]=='%') continue;if (format[index]=='-') ++index;while (index<format.size() && format[index]>='0' && format[index]<='9') ++index; // 원문 문자열 폭은 유지한다.
        if (index<format.size() && format[index]=='.') { ++index;while (index<format.size() && format[index]>='0' && format[index]<='9') ++index; } // 문자열 정밀도도 BYTE 단위로 CRT에 위임한다.
        if (index>=format.size() || format[index]!='s' || ++strings>2) throw std::invalid_argument("동맹 안내의 두 문자열 외 서식");
    }
    const auto left=CurrentMessageName(first),right=CurrentMessageName(second);const std::string pattern(format);char output[256];const auto count=std::snprintf(output,sizeof(output),pattern.c_str(),left.c_str(),right.c_str());if (count<0 || static_cast<std::size_t>(count)>=sizeof(output)) throw std::length_error("동맹 안내 원문256BYTE 서식 범위");return {output,static_cast<std::size_t>(count)};
}
// 원문 문자열+signed금액 또는 signed금액 한 인자만 실제 CRT에 전달한다.
std::string MoneyMessage(std::string_view format,std::span<const char> name,std::int32_t amount,bool withName) {
    const std::string pattern(format.substr(0,format.find('\0')));const std::string_view expected=withName ? "sd":"d";std::size_t consumed{}; // 번역이 요청할 수 있는 실제 인자의 형식/순서다.
    for (std::size_t index=0;index<pattern.size();++index) if (pattern[index]=='%') { // 미공급 가변폭/%n/추가 인자를 원문 지역버퍼 밖 쓰기로 실행하지 않는다.
        if (++index>=pattern.size()) throw std::invalid_argument("SP 안내 서식의 잘린 인자");if (pattern[index]=='%') continue;if (pattern[index]=='-') ++index;while (index<pattern.size() && pattern[index]>='0' && pattern[index]<='9') ++index; // 원문의 고정 폭을 유지한다.
        if (index<pattern.size() && pattern[index]=='.') { ++index;while (index<pattern.size() && pattern[index]>='0' && pattern[index]<='9') ++index; } // 정밀도도 실제 CRT 바이트 단위다.
        if (index>=pattern.size() || consumed>=expected.size() || (pattern[index]!=expected[consumed] && !(expected[consumed]=='d' && pattern[index]=='i'))) throw std::invalid_argument("SP 안내 실제 인자 형식/순서 범위");++consumed;
    }
    char output[256];const auto player=withName ? CurrentMessageName(name):std::string{};const auto count=withName ? std::snprintf(output,sizeof(output),pattern.c_str(),player.c_str(),amount):std::snprintf(output,sizeof(output),pattern.c_str(),amount);if (count<0 || static_cast<std::size_t>(count)>=sizeof(output)) throw std::length_error("SP 안내 원문256BYTE 서식 범위");return {output,static_cast<std::size_t>(count)}; // 서식 뒤 첫 NUL까지 같은 StyleText에 전달한다.
}
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
    if (kind!=kPause && kind!=kGameTime && kind!=kWarning && kind!=kAllied && kind!=kStarted && kind!=kServerInactive && kind!=kPlayerInactive && kind!=kMoneyReceived && kind!=kResourceInjection && kind!=kPresent && kind!=kDisconnected && kind!=kDrawRequested && kind!=kAllianceBroken && kind!=kCompleted) throw std::logic_error("다른 원본 메시지 종류의 생성 분기 미복원");
    hooks_.constructText(token);
    Node(token).vtable=kMessageVtable;
    Node(token).deadline=0.0;
    Node(token).kind=kind;
    Node(token).player=player;
    Node(token).parameter=parameter;
    std::span<const char> firstName; // 이름을 서식에 쓰는 종류만 원문 첫 주소를 번역 뒤까지 보유한다.
    if (kind==kAllied || kind==kPlayerInactive || kind==kMoneyReceived || kind==kPresent || kind==kDisconnected || kind==kDrawRequested || kind==kAllianceBroken) {
        const auto& name=hooks_.challenge() ? hooks_.challengePlayerText : hooks_.playerText;if (!name) throw std::logic_error("동맹 안내의 실제 이름 주소 미연결");firstName=name(player);
    } else if (hooks_.challenge()) {
        if (!hooks_.challengePlayerName) throw std::logic_error("Challenge 메시지의 실제42bbb0 이름 조회 미연결");
        hooks_.challengePlayerName(player);
    } else hooks_.playerName(player);
    if (Node(token).kind==kPresent || Node(token).kind==kDisconnected) {
        if (Node(token).kind==kDisconnected) { //종류5만 공통 이름 조회 뒤 경고음을 재생하며 메타 모드를 다시 검사하지 않는다.
            if (!hooks_.warningSound) throw std::logic_error("연결 해제 안내의 실제 경고 음향 미연결");hooks_.warningSound();const auto owner=Node(token).player;
            if (owner>=1 && owner<=8) { const auto pattern=std::string("~E~")+kAlliedColors[static_cast<std::size_t>(owner-1)]+"%s~w has disconnected";const auto translated=hooks_.translate(pattern);hooks_.style(token,FormatWarningMessage(translated,CurrentMessageName(firstName))); }
        } else if (hooks_.challenge()) { //원문4d0b64는 최초 이름 선택 뒤 현재 메타 플래그를 다시 읽는다.
            const auto translated=hooks_.translate("~E~w%s is present");hooks_.style(token,FormatWarningMessage(translated,CurrentMessageName(firstName)));
        } else {
            if (!hooks_.battle) throw std::logic_error("접속 안내의 현재 일반 전투 관문 미연결");
            if (hooks_.battle()) { const auto owner=Node(token).player;if (owner>=1 && owner<=8) { const auto pattern=std::string("~E~")+kAlliedColors[static_cast<std::size_t>(owner-1)]+"%s~w is present";const auto translated=hooks_.translate(pattern);hooks_.style(token,FormatWarningMessage(translated,CurrentMessageName(firstName))); } }
        }
        if (!hooks_.wallSeconds || !hooks_.idle) throw std::logic_error("접속/이탈 안내의 실제 기한/Idle 미연결");Node(token).deadline=hooks_.wallSeconds()+10.0;hooks_.idle(token,true); //무전투/범위 밖 owner도 원문 공통10초/Idle 꼬리를 실행한다.
    } else if (Node(token).kind==kMoneyReceived || Node(token).kind==kResourceInjection) {
        if (Node(token).kind==kMoneyReceived) { if (!hooks_.warningSound) throw std::logic_error("송금 안내의 실제 경고 음향 미연결");hooks_.warningSound();const auto owner=Node(token).player;if (owner>=1 && owner<=8) { const auto pattern=std::string("~E~")+kAlliedColors[static_cast<std::size_t>(owner-1)]+"%s~w sent you %d Storm Power";const auto translated=hooks_.translate(pattern);hooks_.style(token,MoneyMessage(translated,firstName,parameter,true)); } }
        else { const auto text=hooks_.translate("~E~wResource Injection of %d sp was received");hooks_.style(token,MoneyMessage(text,{},Node(token).parameter,false)); } // 종류18은 음향 없이 번역 뒤 현재+b8을 읽는다.
        if (!hooks_.wallSeconds || !hooks_.idle) throw std::logic_error("SP 안내의 실제 기한/Idle 미연결");Node(token).deadline=hooks_.wallSeconds()+10.0;hooks_.idle(token,true);
    } else if (Node(token).kind==kServerInactive || Node(token).kind==kPlayerInactive) {
        if (!hooks_.warningSound) throw std::logic_error("응답 지연 안내의 실제 경고 음향 미연결");hooks_.warningSound();
        if (Node(token).kind==kServerInactive) hooks_.style(token,hooks_.translate("~wServer not responding"));
        else { const auto owner=Node(token).player;if (owner>=1 && owner<=8) { const auto pattern=std::string("~E~")+kInactiveColors[static_cast<std::size_t>(owner-1)]+"%s~w not responding";const auto translated=hooks_.translate(pattern);hooks_.style(token,FormatWarningMessage(translated,CurrentMessageName(firstName))); } } // 번역 뒤 보관했던 실제 이름 포인터의 현재 NUL까지 읽는다.
    } else if (Node(token).kind==kDrawRequested || Node(token).kind==kAllianceBroken) {
        const auto broken=Node(token).kind==kAllianceBroken; //음향 효과 전 선택된 생성 분기를 보관하며 효과 뒤 kind 변경으로 다른 case를 실행하지 않는다.
        if (broken) { if (!hooks_.warningSound) throw std::logic_error("동맹 해제 안내의 실제 경고 음향 미연결");hooks_.warningSound(); } //종류6은 원문처럼 경고음이 없다.
        const auto row=Node(token).player;
        if (!broken) {
            if (row>=1 && row<=8) { const auto pattern=std::string("~E~")+kAlliedColors[static_cast<std::size_t>(row-1)]+"%s~w wishes to declare a Draw";const auto translated=hooks_.translate(pattern);hooks_.style(token,FormatWarningMessage(translated,CurrentMessageName(firstName))); }
        } else {
            const auto column=Node(token).parameter; //경고음 효과 뒤 현재 두 색 번호를 선택한다.
            if (row>=1 && row<=8 && column>=1 && column<=8) {
                const auto pattern=std::string("~E~")+kAlliedColors[static_cast<std::size_t>(row-1)]+"%s~w and ~"+kAlliedColors[static_cast<std::size_t>(column-1)]+"%s~w have broken their Alliance";
                const auto translated=hooks_.translate(pattern);if (!hooks_.playerText) throw std::logic_error("동맹 해제 안내의 실제 두번째 이름 미연결");const auto second=hooks_.playerText(Node(token).parameter);hooks_.style(token,AlliedMessage(translated,firstName,second)); //번역 뒤 현재 parameter의 이름 주소를 다시 구한다.
            }
        }
        if (!hooks_.wallSeconds || !hooks_.idle) throw std::logic_error("동맹 협상 안내의 실제 기한/Idle 미연결");Node(token).deadline=hooks_.wallSeconds()+10.0;hooks_.idle(token,true); //범위 밖 색도 같은 공통10초/Idle/Fit 꼬리를 수행한다.
    } else if (Node(token).kind==kAllied || Node(token).kind==kStarted || Node(token).kind==kCompleted) {
        if (Node(token).kind==kCompleted) { const auto text=hooks_.translate("~E~wGame completed!");hooks_.style(token,text); } //PE4d2cdc의kind10은4d2bf3→516044이며경고음없이공통10초/Idle/Fit을실행한다.
        else if (Node(token).kind==kStarted) { const auto text=hooks_.translate("~E~wGame started!");hooks_.style(token,text); }
        else {
            if (!hooks_.warningSound) throw std::logic_error("동맹 안내의 실제 경고 음향 미연결");hooks_.warningSound();const auto row=Node(token).player,column=Node(token).parameter;
            if (row>=1 && row<=8 && column>=1 && column<=8) { // 원문 두 switch의 유효1..8에서만 번역/두 번째 이름/Style을 수행한다.
                const auto pattern=std::string("~E~")+kAlliedColors[static_cast<std::size_t>(row-1)]+"%s~w and ~"+kAlliedColors[static_cast<std::size_t>(column-1)]+"%s~w are now Allies";const auto translated=hooks_.translate(pattern);if (!hooks_.playerText) throw std::logic_error("동맹 안내의 실제 두 번째 Player 이름 미연결");const auto second=hooks_.playerText(Node(token).parameter);hooks_.style(token,AlliedMessage(translated,firstName,second)); // PE는 번역 뒤 현재+b8을 재조회하며 앞서 고른 서식 열과 구별한다.
            }
        }
        if (!hooks_.wallSeconds || !hooks_.idle) throw std::logic_error("일반 전투 안내의 실제 기한/Idle 미연결");const auto now=hooks_.wallSeconds();Node(token).deadline=now+10.0;hooks_.idle(token,true);
    } else if (Node(token).kind==kGameTime) {
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
