// 원문4789b0의각호출위치·현재QA/CS재조회와x86signed산술을유지한다.
#include "client/NativeMetaObjectInput.h"
#include <bit>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace netstorm::client { //표시용현재객체슬롯을그대로읽고선택/통신성공을합성하지않는다.
std::optional<std::uint32_t> MetaObjectInputCommandValue(std::uint32_t originalAddress) { switch (originalAddress) { case 0x542754U:return 0x02000017U;case 0x542ac0U:return 0x020000f3U;case 0x5428a4U:return 0x0200006bU;case 0x542a94U:return 0x020000e8U;default:return {}; } } //보호 PE의 명령 값이며 명령 처리 완료나 네트워크 송신을 합성하지 않는다.
namespace { //선택한필드/장치와하위DWORD연산만공통처리한다.
constexpr std::array<std::uint32_t,9> kSeats{4,1,7,2,0xffffffffU,3,6,0,5}; //00540b88의단일읽기전용참조표이며가운데는좌석이없다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("메타 객체 입력의 선택된 실제 장치 미연결");return value; } //배율거부/각분기에서미사용장치를요구하지않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 객체 입력의 현재 필드 물리 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //선택한BYTE폭만낮은자리부터조립한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } //원문signedDWORD해석이다.
std::int32_t Add(std::int32_t left,std::int32_t right) { return Signed(std::bit_cast<std::uint32_t>(left)+std::bit_cast<std::uint32_t>(right)); } //ADD/LEA의하위DWORD감김이다.
std::int32_t Sub(std::int32_t left,std::int32_t right) { return Signed(std::bit_cast<std::uint32_t>(left)-std::bit_cast<std::uint32_t>(right)); } //SUB의하위DWORD감김이다.
std::int32_t Cell(std::int32_t hotspot,std::int32_t mouse,std::int32_t origin,std::int32_t extent) { const auto value=Signed(std::bit_cast<std::uint32_t>(Sub(Add(hotspot,mouse),origin))*3U);if (!extent || (value==std::numeric_limits<std::int32_t>::min() && extent==-1)) throw std::domain_error("메타 좌석의 Native IDIV 오류 경계");const auto index=value/extent;return index<0 ? 0:index>1 ? 2:index; } //signedIDIV뒤원문의0/1/2분기를적용하며먼저좌표를clamp하지않는다.
}
// 참가자와전투의호출분기는배율0/10000h인경우에만선택한다.별도kill/active관문은없다.
bool InputNativeMetaObject(std::uint32_t sid,std::span<const std::uint8_t> object,InputEvent event,NativeMetaObjectInputGlobals globals,const NativeMetaObjectInputHooks& hooks) {
    const auto fixed=((Read(object,0x1a)&0x3fffU)<<16)+Read(object,8,2);if (fixed && fixed!=0x10000U) return false;
    if (Read(object,10,1)!=globals.challenge.BattleType()) { Need(hooks.playerMenu)(Read(object,0xc,2),{event.x,event.y});return true; }
    if ((event.code&0x03ffffffU)==0x20000U) {
        Need(hooks.begin)();auto title=Need(hooks.typeName)();title.push_back('\n');if (title.size()>=256) throw std::length_error("원문 메타 제목의256BYTE 물리 범위");Need(hooks.title)(title);
        if (!Need(hooks.helpWord)()) Need(hooks.assertion)("helpWord",".\\Metadisplay.cpp",0x1fe);
        const auto type=Read(object,10,1),helpCommand=Need(hooks.command)(0x542754U);const auto help=Need(hooks.helpWord)();if (!help) throw std::logic_error("메타 제목 진단복귀 뒤 Native NULL helpWord 접근");Need(hooks.item)(*help,helpCommand,type);
        const auto own=globals.challenge.Participant(globals.localOwner);const auto battle=Read(own,0xa4);
        if (battle && battle==Read(object,0xc,2) && (Read(own,0x64,1)&4U)) { const auto parameter=Need(hooks.descriptionParameter)(battle);const auto text=Need(hooks.translate)("Change Description...");Need(hooks.item)(text,Need(hooks.command)(0x542ac0U),parameter); }
        Need(hooks.reference)(sid,{event.x,event.y});return true;
    }
    const auto point=Need(hooks.project)(std::bit_cast<float>(Read(object,0xe)),std::bit_cast<float>(Read(object,0x12)));const auto frame=Need(hooks.frameZero)(Read(object,10,1));const auto column=Cell(frame[2],event.x,point.x,frame[0]),row=Cell(frame[3],event.y,point.y,frame[1]);const auto seat=kSeats[static_cast<std::size_t>(column+row*3)];
    if (seat!=0xffffffffU) {
        const auto battle=Read(object,0xc,2);const auto record=globals.challenge.Battle(battle);
        if (Signed(Read(record,0x18))<79 && Signed(Read(record,0x1c))>77 && !Read(record,0x38+seat*4)) {
            const auto ownBattle=Read(globals.challenge.Participant(globals.localOwner),0xa4),parameter=(battle<<8)|seat;
            if (ownBattle && ownBattle!=battle) { Need(hooks.query)("QueryLeaveJoinChalBattle("+std::to_string(parameter)+")");return true; }Need(hooks.queue)(Need(hooks.command)(0x5428a4U),0,parameter);
        }
    }
    return true;
}
//영역 기록 주소는 첫 QA로 고정하고 번역/제목 작성 뒤 같은 기록의 현재 번호를 다시 읽는다.
bool InputNativeMetaZoneObject(std::uint32_t sid,std::span<const std::uint8_t> object,InputEvent event,std::span<const std::uint8_t> zones,const NativeMetaObjectInputHooks& hooks) {
    const auto fixed=((Read(object,0x1a)&0x3fffU)<<16)+Read(object,8,2);if (fixed && fixed!=0x10000U) return false;
    Need(hooks.begin)();const auto offset=Read(object,0xc,2)*144U;if (offset>zones.size() || 144>zones.size()-offset) throw std::out_of_range("메타 영역 메뉴의 실제51기록 물리 범위");const auto region=zones.subspan(offset,144);
    std::array<char,256> text;const auto pattern=Need(hooks.translate)("Challenge Area (id %d)\n");const auto count=std::snprintf(text.data(),text.size(),pattern.c_str(),Signed(Read(region,0)));if (count<0 || static_cast<std::size_t>(count)>=text.size()) throw std::length_error("메타 영역 제목의256BYTE 서식 범위");Need(hooks.title)({text.data(),static_cast<std::size_t>(count)});
    const auto label=Need(hooks.translate)("Decend to this Level");const auto command=Need(hooks.command)(0x542a94U);Need(hooks.item)(label,command,Read(region,0));
    if (!Need(hooks.helpWord)()) Need(hooks.assertion)("helpWord",".\\Metadisplay.cpp",0x390);const auto type=Read(object,10,1);const auto helpCommand=Need(hooks.command)(0x542754U);const auto help=Need(hooks.helpWord)();if (!help) throw std::logic_error("메타 영역 진단복귀 뒤 Native NULL helpWord 접근");Need(hooks.item)(*help,helpCommand,type);
    Need(hooks.reference)(sid,{event.x,event.y});return true;
}
//확인만 요청하며 실제 ViewMeta/상태 전환이나 현재 서버 선택을 앞당기지 않는다.
bool DispatchNativeMetaZoneRequest(InputEvent event,const std::function<void(std::string_view)>& tell) { if ((event.code&0x03ffffffU)!=0x020000e8U) return false;if (!(event.code&InputCode::kRelease)) Need(tell)("QueryDecendToChalLevel("+std::to_string(event.y)+")");return true; }
}
