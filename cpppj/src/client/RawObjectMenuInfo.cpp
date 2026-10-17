// 제목/정보의실제번역·현재값읽기·서식·메뉴효과순서를보존한다.
#include "client/RawObjectMenuInfo.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 제목/이름/현재게임조회누락을빈문구나가짜성공으로대체하지않는다.
template<class F> const F& Required(const F& hook) { if (!hook) throw std::logic_error("객체메뉴정보 실제하위미연결");return hook; }
}
// 판본과각현재배열크기를검증하며생성중메뉴/게임효과를실행하지않는다.
RawObjectMenuInfo::RawObjectMenuInfo(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const std::string> classes,ObjectMenuInfoState& state,ObjectMenuInfoHooks hooks)
    :pool_(pool),types_(types),classes_(classes),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188 || classes.size()!=188) throw std::invalid_argument("객체메뉴정보10.78 풀/TYPE/클래스표오류");
}
// 부모원본에없는free/dead/void필터를추가하지않는다.
std::span<const std::uint8_t> RawObjectMenuInfo::Raw(o::Sid object) const {
    if (!object.value || object.value>=pool_.Capacity()) throw std::out_of_range("객체메뉴정보 SID범위오류");return pool_.Slot(object);
}
// callback뒤에도같은물리TYPE의현재필드를읽는다.
const o::RiftTypeRecord& RawObjectMenuInfo::Type(std::uint32_t number) const {
    if (number>=types_.size()) throw std::out_of_range("객체메뉴정보 TYPE범위오류");return types_[number];
}
// 실제TYPE초기화의class길이23한도를확인하고C문자열의첫NUL까지만노출한다.
std::string_view RawObjectMenuInfo::Class(std::uint32_t number) const {
    Type(number);std::string_view text=classes_[number];text=text.substr(0,text.find('\0'));
    if (text.size()>23) throw std::length_error("객체메뉴정보 원본class23바이트초과");return text;
}
// 원본고정버퍼는NUL을포함하므로같은capacity이상의출력을거부한다.
std::string RawObjectMenuInfo::Text(std::string value,std::size_t capacity) {
    const auto nul=value.find('\0');if (nul!=std::string::npos) value.resize(nul);
    if (value.size()>=capacity) throw std::length_error("객체메뉴정보 원본문자열버퍼초과");return value;
}
// 서식의번역여부와호출시점은호출자가선택하며이함수는추가번역하지않는다.
std::string RawObjectMenuInfo::Print(std::string_view pattern,std::initializer_list<PriestMenuArgument> arguments) const {
    return Text(Required(hooks_.print)(pattern,{arguments.begin(),arguments.size()}),256);
}
// 좌표는원본little endian DWORD로조립한다.
std::uint32_t RawObjectMenuInfo::Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 필요한4바이트만원본의낮은바이트부터읽는다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);return value;
}
// 불확정64비트의하위0과정상값의32비트감김을보존한다.
std::int32_t RawObjectMenuInfo::Truncate(float value) {
    const double wide=value;if (!std::isfinite(wide) || wide<-0x1p63 || wide>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(wide)));
}
// slash문자열을한번분리하는원본의반환문자열과끝sentinel을값으로표현한다.
std::string RawObjectMenuInfo::LevelName(std::uint32_t number) {
    constexpr std::array<std::string_view,10> names{"I","II","III","IV","V","VI","VII","VIII","IX","X"}; // PE542420의실제원문순서다.
    return number<names.size() ? std::string(names[number]) : std::string(1,static_cast<char>(0xff));
}
// 원본542444의8포인터순서를보존한다. Describe는첫5개또는None만사용한다.
std::string_view RawObjectMenuInfo::ThemeName(std::int32_t number) {
    constexpr std::array<std::string_view,8> names{"Wind","Rain","Thunder","Sun","Energy","Nature","Holy","Evil"}; // 실제보호PE포인터문자열이다.
    if (number<0 || static_cast<std::size_t>(number)>=names.size()) throw std::out_of_range("객체메뉴정보 원소이름표범위오류");return names[static_cast<std::size_t>(number)];
}
// 네인자는독립값이다. 외부효과사이의현재class/flags/theme/모드는해당원본위치에서다시읽는다.
void RawObjectMenuInfo::Describe(std::uint32_t type,std::uint32_t count,std::uint32_t owner,std::uint32_t sid) const {
    Type(type);Required(hooks_.title)(Text(Required(hooks_.description)(type),256));
    if (type==state_.altar) --count;
    if (!Class(type).empty() && (Type(type).flags2&0x80074000U)) {
        const auto pattern=Required(hooks_.translate)(" Level %s");const auto level=LevelName(std::bit_cast<std::int32_t>(count)<0 ? 0U : count);
        Required(hooks_.title)(Print(pattern,{level}));
    }
    if (type==state_.nugget) owner=0;
    const auto name=Text(std::bit_cast<std::int32_t>(owner)>0 ? Required(hooks_.ownerName)(owner) : Required(hooks_.translate)("Nobody"),80);
    const auto local=Required(state_.localOwner)();std::string relation; // 현재로컬번호는이름조회뒤한번읽는다.
    if (owner==local || !owner) relation=Required(hooks_.translate)("Owner");
    else relation=Required(hooks_.translate)(Required(hooks_.alliance)(owner,local) ? "Ally" : "Enemy");
    relation=Text(std::move(relation),256);
    if (Required(state_.editor)() && sid!=0xffffffffU) {
        if (sid>65535U) throw std::out_of_range("객체메뉴정보 편집SID표현범위초과");const o::Sid object{static_cast<std::uint16_t>(sid)};Raw(object);
        Required(hooks_.header)(Print("\n%s: ~i%d~.",{std::string("SubID"),std::bit_cast<std::int32_t>(sid)}));
        const auto raw=Raw(object);const auto x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));
        const auto iy=Truncate(y),ix=Truncate(x);Required(hooks_.header)(Print("\nAt (~i%d~w,~i%d~w)~.",{ix,iy}));
    }
    Required(hooks_.header)(Print("\n%s: ~i%s~.",{relation,name}));
    if (!Class(type).empty()) {
        const auto alignmentPattern=Required(hooks_.translate)("\nAlignment: ~i%s~.");const auto theme=Type(type).theme;
        const auto alignment=Required(hooks_.translate)(theme<5 ? ThemeName(theme) : "None");Required(hooks_.header)(Print(alignmentPattern,{alignment}));
        const auto classPattern=Required(hooks_.translate)("\nClass: ~i%s~.");const auto translated=Required(hooks_.translate)(Class(type));
        const auto substituted=Text(Required(hooks_.substitute)(translated),1024);Required(hooks_.header)(Print(classPattern,{substituted}));
    }
    Required(hooks_.header)("\n");
}
// 원본4ada70은count의외부효과전에owner/TYPE를보관한다. 그후경고extra는현재raw를다시읽는다.
void RawObjectMenuInfo::Summary(o::Sid object) const {
    const auto raw=Raw(object);const auto owner=raw[34],type=raw[10];const auto count=Required(hooks_.count)(object);Describe(type,count,owner,object.value);
    if (Raw(object)[40]&0x10) { const auto pattern=Required(hooks_.translate)("~y~[I%d.*]Invisible!~.\n");Required(hooks_.header)(Print(pattern,{std::bit_cast<std::int32_t>(state_.invisible)})); }
    if (Raw(object)[40]&0x20) { const auto pattern=Required(hooks_.translate)("~y~[I%d.*]Paralyzed!~.\n");Required(hooks_.header)(Print(pattern,{std::bit_cast<std::int32_t>(state_.paralyzed)})); }
    CaptureStatus(object);
}
// 492090이거짓이면현재미션/Player표를읽지않는다. 참이면원본의short-circuit순서다.
void RawObjectMenuInfo::CaptureStatus(o::Sid object) const {
    Raw(object);if (!Required(hooks_.groundImmobile)(object)) return;
    const bool disconnected=!Required(state_.missionPost)() && !(Required(hooks_.ownerStatus)(Raw(object)[34])&1);
    Required(hooks_.header)(Required(hooks_.translate)(disconnected ? "~yNot Connected!~.\n" : "~yCapturable!~.\n"));
}
// 같은controlled메뉴의제목/머리몸체를호출하며별도문구·항목·레이아웃을생성하지않는다.
void BindObjectMenuInfoControls(ObjectMenuInfoHooks& hooks,const MenuGumpControls& controls) {
    hooks.title=[&controls](std::string_view text) { controls.AddTitle(text); };hooks.header=[&controls](std::string_view text) { controls.AddHeader(text); };
}
}
