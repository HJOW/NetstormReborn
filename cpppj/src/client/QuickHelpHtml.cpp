// 태그/이름의 원문 case와 주제 prefix·현재 TYPE 재조회·정보 pane의 실제 호출 순서를 유지한다.
#include "client/QuickHelpHtml.h"
#include "client/AiTypeRequiredEnergy.h"
#include "o/OriginalText.h"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace netstorm::client {
// 설치된 월드는 Session의 현재 표를 사용한다. 월드가 없을 때만 같은 전역 TYPE/AST의 초기 값을 읽는다.
QuickHelpTypeInfo ReadAssetQuickHelpType(const GameAssets& assets,std::uint32_t type) {
    const auto types=assets.TypeTable().Types();if (type>=types.size()) throw std::out_of_range("월드 설치 전 Help TYPE 범위");const auto& record=types[type];QuickHelpTypeInfo result;
    result.theme=record.theme;result.flags1=record.flags1;result.flags2=record.flags2;result.cost=record.cost;result.mana=BindAiTypeRequiredEnergy(assets,types)(type);
    if (type>=o::kFirstAssetTypeNumber && type-o::kFirstAssetTypeNumber<assets.Types().size()) {
        const auto& definition=assets.Types()[type-o::kFirstAssetTypeNumber].definition;result.range=static_cast<float>(definition.Number("range").value_or(0));result.damage=static_cast<float>(definition.Number("hpPerSec").value_or(0));
        if (const auto classification=definition.String("class")) result.classification=classification->substr(0,classification->find('\0'));if (result.classification.size()>23) throw std::length_error("월드 설치 전 Help class23바이트 초과");
    }
    return result;
}
namespace {
// 선택한 실제 효과가 없으면 빈 화면으로 성공시키지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("전용 Help HTML 실제 효과 누락");return hook; }
// C문자열의 첫NUL 뒤는 읽지 않는다.
std::string_view CText(std::string_view text) { return text.substr(0,text.find('\0')); }
// 원본 고정 지역 버퍼의 NUL 공간을 확인한다.
std::string Text(std::string value,std::size_t capacity) { value.resize(CText(value).size());if (value.size()>=capacity) throw std::length_error("전용 Help HTML 원본 버퍼 초과");return value; }
// 원본 ftol의 signed64 절삭 뒤 하위 DWORD를 읽는다.
std::int32_t Truncate(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-0x1p63 || static_cast<double>(value)>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 실제 마나 초기화의 원문 문자표이며 알 수 없는 ASCII는 초기0 프레임이다.
ManaPictureState::ManaPictureState() {
    constexpr std::string_view letters="wrtsWRTS.?>"; // PE00540ad8이 가리키는 원래11문자다.
    // 문자 위치가 바로 SHP 프레임 번호다.
    for (std::size_t i=0;i<letters.size();++i) frames[static_cast<std::size_t>(letters[i])]=static_cast<std::int32_t>(i);
}
// point 수와 별개로 실제 비점 문자마다 하나의 그림을 추가한다.
std::string ManaPictureText(std::string_view mana,const ManaPictureState& state,std::int32_t highlighted) {
    mana=CText(mana);if (mana.size()>=256) throw std::length_error("원본 마나 임시 버퍼256바이트 초과");std::string result;std::size_t cursor{};std::int32_t index{};
    // 점은 원본처럼 연속으로 건너뛰며 강조 인덱스는 출력한 아이콘 수로 증가한다.
    while (cursor<mana.size()) {
        // 연속한 점은 마나 하나로 세지 않고 다음 비점 문자 앞까지 소비한다.
        while (cursor<mana.size() && mana[cursor]=='.') ++cursor;if (cursor==mana.size()) break;
        const auto letter=static_cast<unsigned char>(mana[cursor++]);if (letter>=state.frames.size()) throw std::out_of_range("마나 비ASCII 전역 조회 미복원");
        const auto frame=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state.frames[letter])+(index<highlighted ? 11U : 0U));result+="~[I"+std::to_string(state.type)+'.'+std::to_string(frame)+']';++index;
    }
    if (result.size()>=250) throw std::length_error("원본 마나 그림 문자열250바이트 초과");return result;
}
// 현재 입력 바로 앞 바이트를 조회하며 태그 전체를 해석하거나 shown을 바꾸지 않는다.
void QuickHelpBeforeClose(HtmlTextParserState& state,std::string_view source) {
    if (!state.inputOffset || state.inputOffset>source.size()) throw std::out_of_range("Help 닫힘 슬롯의 현재 입력 범위");
    const auto previous=source[state.inputOffset-1];if (previous=='\n' || previous=='\r') state.textAllowed=0;
}
// 실제 파생 생성자는 기본 shown1을0으로 바꾸고 주제/type/pane/stream만 추가로 설치한다.
QuickHelpHtml::QuickHelpHtml(HtmlTextParserState& state,HtmlTextParserGlobals& globals,std::uint32_t& infoMode,
    std::uint32_t type,std::string_view topic,bool stream,ManaPictureState mana,QuickHelpHtmlHooks hooks,HtmlTextParserHooks base,HtmlLinkTextState* links)
    :state_(state),globals_(globals),infoMode_(infoMode),type_(type),topic_(CText(topic)),stream_(stream),mana_(std::move(mana)),hooks_(std::move(hooks)),base_(std::move(base)),links_(links) {
    if (base_.link && !links_) throw std::invalid_argument("Help 중첩 HTML의 같은 링크 저장소 누락");state_=HtmlTextParserState{};
    if (type_) static_cast<void>(Need(hooks_.type)());state_.shown=0; // 실제 생성자는 본문 선택 전에 TYPE를 조회한다.
}
// 번역은 각 원본 호출 위치에서 명시하며 서식기는 추가 번역을 하지 않는다.
std::string QuickHelpHtml::Print(std::string_view pattern,std::initializer_list<PriestMenuArgument> arguments,std::size_t capacity) const { return Text(Need(hooks_.print)(pattern,{arguments.begin(),arguments.size()}),capacity); }
// 제목·원소·클래스·HP·거리·초당 피해·SP·마나 순서로 현재 TYPE를 읽는다.
void QuickHelpHtml::Info() const {
    const auto current=[&]() -> const QuickHelpTypeInfo& { return Need(hooks_.type)(); }; // 각 실제 읽기 시점의 같은 TYPE다.
    const auto translate=[&](std::string_view value) { return Need(hooks_.translate)(value); }; // 원본 번역 순서를 유지한다.
    const auto heading=Print("<h2>%s</h2>",{Need(hooks_.description)()});std::string alignment;
    if (current().theme<5) {
        constexpr std::array<std::string_view,5> names{"Wind","Rain","Thunder","Sun","Energy"}; // 실제542444 첫5포인터다.
        const auto pattern=translate("Alignment: <c>%s</c>");const auto theme=current().theme;if (theme<0 || theme>=5) throw std::out_of_range("Help 원소 이름표 범위");alignment=Print(pattern,{translate(names[static_cast<std::size_t>(theme)])});
    } else alignment=Print(translate("Alignment: <c>None</c>"),{});
    std::string classification;
    if (!CText(current().classification).empty()) { const auto pattern=translate("Class: <c>%s</c>");const auto label=translate(CText(current().classification));classification=Print(pattern,{Text(Need(hooks_.substitute)(label),1024)}); }
    std::string hits;
    if ((current().flags1&0x10U) && Need(hooks_.hitPoints)()) { const auto pattern=translate("Hits: <c>%d</c>");hits=Print(pattern,{Need(hooks_.hitPoints)()}); }
    else hits=Print(translate("Hits: <c>Invulnerable</c>"),{});
    const auto number=[&](std::string_view numeric,std::string_view empty,const std::function<float()>& value) { // 번역 뒤 현재 float을 읽어 숫자로 절삭한다.
        if (value()==0.0f) return Print(translate(empty),{});const auto pattern=translate(numeric);return Print(pattern,{Truncate(value())});
    };
    const auto range=number("Range: <c>%d</c>","Range: <c>n/a</c>",[&] { return current().range; });std::string damage;
    if (current().damage==0.0f) damage=Print(translate((current().flags2&0x100U) ? "Damage: <c>special</c>" : "Damage: <c>n/a</c>"),{});
    else { const auto pattern=translate("Damage: <c>%d</c>");damage=Print(pattern,{Truncate(current().damage)}); }
    const auto cost=number("Cost in Storm Power: <c>%d</c>~[IfortGump.3]","Cost in Storm Power: <c>n/a</c>",[&] { return current().cost; });std::string mana;
    if (CText(current().mana).empty()) { const auto pattern=translate((current().flags2&0x100U) ? "%s to Build: <c>n/a</c>" : "%s to Build: <c>None</c>");mana=Print(pattern,{Need(hooks_.config)("influence")}); }
    else {
        auto letters=Text(current().mana,256);
        // 원본473090은256바이트까지 ASCII 대문자로 바꾼다.
        for (auto& letter:letters) if (letter>='a' && letter<='z') letter=static_cast<char>(letter-'a'+'A');
        const auto pictures=ManaPictureText(letters,mana_);const auto pattern=translate("%s to Build: <c>%s</c>");mana=Print(pattern,{Need(hooks_.config)("influence"),pictures});
    }
    const auto html=Print("%s%s<br>%s<br>%s<br>%s<br>%s<br>%s<br>%s<br>",{heading,alignment,classification,hits,range,damage,cost,mana},1024);
    if (stream_) Need(hooks_.stream)(html);HtmlTextParserState state;HtmlTextParser parser(state,globals_,base_);const auto cursor=links_ ? links_->cursor : 0U;
    infoMode_=0;const auto styled=parser.Parse(html,1024);infoMode_=1;if (links_) links_->cursor=cursor; // 중첩 정보 파서의 +10은 부모의 현재 포인터 복사본이다.
    Need(hooks_.paneText)(styled);Need(hooks_.paneScroll)(0);auto* pane=Need(hooks_.pane)();if (!pane) throw std::out_of_range("Help 현재 정보 pane 누락");
    if (!(pane->flags&8U)) { Need(hooks_.panePush)();Need(hooks_.paneAttach)(); }
}
// name의 검색 중 바뀐 cursor는 원본처럼 가상 슬롯 진입 위치로 다시 돌려놓는다.
void QuickHelpHtml::Inside(HtmlTextParserState& state,std::string_view source,std::int32_t capacity) const {
    const auto entry=state.inputOffset;auto position=entry;
    if (position<source.size() && source[position]=='n' && o::AsciiLower(source.substr(position,4))=="name") {
        // 첫 인용/NUL까지 읽으며 속성의 '='나 공백을 따로 해석하지 않는다.
        while (position<source.size() && source[position]!='"') ++position;if (position<source.size()) ++position;
        if (o::AsciiLower(source.substr(position,topic_.size()))==o::AsciiLower(topic_)) {
            state.shown=1;state.textAllowed=1;if (state.output.size()+4>=static_cast<std::size_t>(capacity)) throw std::length_error("Help name 스타일 출력 범위 초과");state.output+="~5~w";position+=topic_.size();
        }
    }
    if (source.substr(position,4)=="info" && type_ && state.shown) Info();state.inputOffset=entry;
}
// afterTag는 본문 파서의 cursor를 바꾸지 않고 기록용 다음 구간만 읽는다.
void QuickHelpHtml::After(HtmlTextParserState& state,std::string_view source) const {
    if (!stream_) return;auto position=state.inputOffset+1;
    // 다음 '<' 앞 평문은 제어문자/CRLF를 포함해 한 바이트씩 기록한다.
    while (position<source.size() && source[position]!='<') { Need(hooks_.stream)(source.substr(position,1));++position; }
    // 다음 태그는 첫 '>'까지 포함한다. NUL은 파일에 기록하지 않는다.
    while (position<source.size() && (position==0 || source[position-1]!='>')) { Need(hooks_.stream)(source.substr(position,1));++position; }
}
// 같은 기본 명령/조건/그림/링크 저장소를 정보 표와 본문이 함께 사용한다.
std::string QuickHelpHtml::Parse(std::string_view source,std::int32_t capacity) const {
    auto hooks=base_;hooks.insideTag=[this,capacity](auto& state,auto text) { Inside(state,text,capacity); };hooks.beforeClose=QuickHelpBeforeClose;hooks.afterTag=[this](auto& state,auto text) { After(state,text); };
    HtmlTextParser parser(state_,globals_,std::move(hooks));return parser.Parse(source,capacity);
}
}
