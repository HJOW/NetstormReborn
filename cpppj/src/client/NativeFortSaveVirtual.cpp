//실제저장가상읽기의순수몸체를공유하며NoWorld를빈성공/임시World로대체하지않는다.
#include "client/NativeFortSaveVirtual.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawGameAuxState.h"
#include "o/RawSquidLevel.h"
#include "o/RawSquidFinder.h"
#include "o/RawPathAnimation.h"
#include "o/RawAltarSacrifice.h"
#include "o/RawDaisRegular.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <span>
#include <stdexcept>

namespace netstorm::client { //조회는현재Pool/TYPE를직접사용하고등록표/Runtime/권한을선택하지않는다.
namespace { //선택한물리필드와원본좌표변환만처리한다.
//선택한 필드 폭의 비정렬 BYTE/WORD/DWORD를 현재 물리에서 낮은 BYTE부터 읽는다.
std::uint32_t ReadSaveField(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>4 || width>bytes.size()-at) throw std::out_of_range("Fort가상조회의선택필드물리범위부족");
    std::uint32_t result{};for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result; //선택한폭만비정렬little endian으로읽는다.
}
int CountCell(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-2147483648.0 || static_cast<double>(value)>=2147483648.0) throw std::out_of_range("Fort제단Count좌표의실제signedDWORD범위부족");return static_cast<int>(value);
} //기존제단Cell과같이0방향절삭하며잘못된좌표를별도유효칸으로정상화하지않는다.
o::RawSquidLevel CurrentLevelBody(RawSurfaceStorage& storage) {
    return o::RawSquidLevel(storage.pool,storage.types,o::MakeSquidLevelAssertion(storage.pool.Edition(),[&storage](auto expression,auto file,auto line) { storage.AssertDisplay(expression,file,line); }));
} //실제참조만보유하는기존순수level몸체이며ctor/소멸은Pool/Player/Host를변경하지않는다.
}
NativeFortSaveVirtual::NativeFortSaveVirtual(RawSurfaceStorage& storage,const o::RawGameAuxState& globals):storage_(storage),globals_(globals) { if (storage.pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("Fort가상저장adapter는10.78현재자료가필요합니다"); } //명시판본물리계약이며현재소유자만빌리고선택되지않은자산효과는요구하지않는다.
//현재가상표로분배한뒤generic분기에서만현재TYPEflags를읽는다.
std::uint32_t NativeFortSaveVirtual::ObjectRobust(o::Sid sid) const {
    const auto raw=storage_.pool.Slot(sid);const auto table=ReadSaveField(raw,0);
    if (!o::SquidFactory::SupportsConstructedVtable(storage_.pool.Edition(),table)) throw std::logic_error("FortRobust의현재가상표몸체미연결");
    constexpr std::array<std::uint32_t,9> rejected{0x5070c0,0x508cc8,0x5092d0,0x5096a0,0x509758,0x509810,0x50a8e8,0x50db78,0x50ee08}; //기존실제생성범위중항상0인가상60주소정체성이다.
    if (std::find(rejected.begin(),rejected.end(),table)!=rejected.end()) return 0;
    if (table==0x5040c0U || table==0x504198U) {
        const auto frame=std::bit_cast<std::int32_t>(ReadSaveField(raw,36));const auto type=storage_.pool.Slot(sid)[10];
        if (type>=storage_.types.size() || type>=storage_.frames.size()) throw std::out_of_range("Fort벌룬Robust의현재TYPE/frames물리범위부족");
        return frame<static_cast<std::int32_t>(storage_.frames[type].Codes().size()); //같은RawBalloonBody::Robust의현재frame와TYPE+114값이다.
    }
    const auto type=storage_.pool.Slot(sid)[10];if (type>=storage_.types.size()) throw std::out_of_range("FortRobust의현재TYPE물리범위부족");return !(storage_.types[type].flags1&0x20000000U);
}
//현재flags진단이장치를바꿔도복귀뒤같은Pool의현재하위BYTE를다시읽는다.
std::uint32_t NativeFortSaveVirtual::ContentRobust(o::Sid sid) const {
    if (ReadSaveField(storage_.pool.Slot(sid),20,1)&0x80U) storage_.AssertDisplay("!(getListFlags() & CFF_WORLD)","\\Ns\\O\\Squid.cpp",0xbb1);
    return (ReadSaveField(storage_.pool.Slot(sid),20,1)&0x34U)!=0;
}
std::uint32_t NativeFortSaveVirtual::HasLevel(o::Sid sid) const { return CurrentLevelBody(storage_).Has(sid); } //현재TYPE물리검사와기존항상참OR조건을공유한다.
//가상80은제단→현재좌표의Dais재조회와Dais프레임판정만별도로처리한다.
std::uint32_t NativeFortSaveVirtual::Level(o::Sid sid) {
    if (!sid.value || sid.value>=storage_.pool.Capacity()) throw std::out_of_range("Fort가상80의현재SID물리범위부족");
    const auto raw=storage_.pool.Slot(sid);if (raw[10]>=storage_.types.size()) throw std::out_of_range("Fort가상80의현재TYPE물리범위부족");const auto table=ReadSaveField(raw,0);
    if (!o::SquidFactory::SupportsConstructedVtable(storage_.pool.Edition(),table)) throw std::logic_error("Fort가상80의현재가상표몸체미연결");
    if (!activeCounts_.insert(sid.value).second) throw std::logic_error("Fort가상80의현재SID재귀사슬순환");
    struct CountCall { //손상순환의호스트guard만정상/예외복귀에반납하고게임전역은복구하지않는다.
        std::unordered_set<std::uint16_t>& active;std::uint16_t sid; //이조회사슬의집합과이번호출의실제SID다.
        ~CountCall() { active.erase(sid); } //동기조회한프레임의표식만제거한다.
    } call{activeCounts_,sid.value};
    if (table==o::kPatchAltarVtable) {
        const auto pointX=std::bit_cast<float>(ReadSaveField(raw,14)),pointY=std::bit_cast<float>(ReadSaveField(raw,18));const auto type=globals_.DaisType();
        if (type>=storage_.types.size()) throw std::out_of_range("Fort제단Count의현재DaisTYPE물리범위부족");const auto x=CountCell(pointX),y=CountCell(pointY);o::RawSquidFinder finder(storage_.pool,storage_.hash,storage_.types);
        for (auto current=finder.Begin({x,y,x,y},(storage_.types[type].flags2&6U)==0 ? 1U:0U);current.value;current=finder.Next()) { //원문현재공간후보와현재rawTYPE를비교한첫Dais만선택한다.
            if (storage_.pool.Slot(current)[10]==type) return Level(current);
        }
        return 0; //같은원문DaisAt/Valid의NULL후보자연반환이다.
    }
    if (table==o::kPatchDaisVtable) {
        const auto current=storage_.pool.Slot(sid);const auto type=current[10];if (type>=storage_.types.size() || type>=storage_.frames.size()) throw std::out_of_range("FortDaisCount의현재TYPE/frames물리범위부족");
        const auto frame=std::bit_cast<std::int32_t>(ReadSaveField(current,36));const auto direction=o::RawPathAnimation::Direction(storage_.frames[type],frame);
        if (direction==13) return 1;if (direction==12) return 2;if (direction==11) return 3;return frame<64 ? 1U:frame<=119 ? 2U:3U;
    }
    return CurrentLevelBody(storage_).Get(sid); //나머지실제생성가상표는기존base+80현재상태BYTE를읽는다.
}
}
