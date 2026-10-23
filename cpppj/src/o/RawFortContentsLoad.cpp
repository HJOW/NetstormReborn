// 읽으면서즉시만들어callback뒤의현재conversion/TYPE/stream을반영한다.앞선생성·pos를rollback하지않는다.
#include "o/RawFortContentsLoad.h"
#include <bit>
#include <stdexcept>
#include <vector>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kContentsQA=0x1000U,kContentsQB=0x2000U,kContentsContainer=0x20000U; //첫TYPE.flags1에서캡처하는세원본비트다.
// 도달한누락장치만거부하며빈목록의미선택TYPE/Create는요구하지않는다.
template<class Function> const Function& NeedContents(const Function& function) { if (!function) throw std::logic_error("Fort 원시Contents의 실제 경계 미연결");return function; }
// 실제현재16BYTE의DWORD를비정렬little endian으로읽는다.
std::uint32_t ContentsU(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Fort Contents 현재stream DWORD 물리범위");std::uint32_t value{}; //현재네BYTE조립값이다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; //필요한DWORD만읽는다.
}
// 실제pos DWORD만쓰고data/end/capacity는보존한다.
void ContentsPosition(std::span<std::uint8_t> fields,std::uint32_t position) {
    if (fields.size()<8) throw std::out_of_range("Fort Contents 현재pos 물리범위");
    for (std::size_t byte=0;byte<4;++byte) fields[4+byte]=static_cast<std::uint8_t>(position>>(byte*8)); //상위BYTE도원문DWORD폭으로대입한다.
}
// 직접BYTE는논리end를검사하지않고실제버퍼물리끝만거부한다.
std::uint8_t ContentsByteAt(std::span<const std::uint8_t> bytes,std::uint32_t position) { if (position>=bytes.size()) throw std::out_of_range("Fort Contents 직접BYTE 물리범위");return bytes[position]; }
// 현재data/pos에서하나읽은뒤같은현재stream의pos를진행한다.
std::uint8_t ContentsByte(std::span<std::uint8_t> fields,const MemStreamHooks& memory) {
    const auto position=ContentsU(fields,4),data=ContentsU(fields,0);const auto bytes=NeedContents(memory.buffer)(data); //한BYTE의실제주소를먼저캡처한다.
    const auto value=ContentsByteAt(bytes,position);ContentsPosition(fields,position+1U);return value;
}
// QB만현재end를캡처해pos증가뒤signed진단한다.진단이data/pos를바꾸어도읽은WORD값은유지한다.
std::uint32_t ContentsWord(std::span<std::uint8_t> fields,const MemStreamHooks& memory,const FortContentsLoadHooks& hooks) {
    const auto position=ContentsU(fields,4),data=ContentsU(fields,0);const auto bytes=NeedContents(memory.buffer)(data); //원문EAX/ECX의처음주소다.
    if (position>bytes.size() || bytes.size()-position<2) throw std::out_of_range("Fort Contents QB WORD 물리범위");
    const auto word=static_cast<std::uint16_t>(std::uint32_t(bytes[position])|(std::uint32_t(bytes[position+1])<<8)); //진단앞에서캡처한원시16비트다.
    const auto end=ContentsU(fields,8),next=position+2U;ContentsPosition(fields,next); //원문은end를읽고pos를쓴뒤비교한다.
    if (std::bit_cast<std::int32_t>(next)>std::bit_cast<std::int32_t>(end)) NeedContents(hooks.assertion)(FortContentsLoadAssertion::WordPastEnd);
    return std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word)));
}
// 실제재귀의부모/수정된attach flags/남은count를독립프레임으로유지한다.
struct ContentsLoadFrame {
    std::uint32_t parent,flags,remaining; //자식호출이부모의flags/count를덮지않도록각호출값을보관한다.
};
}
// 동기access한번안에서모든원문재귀를실행하며Guard밖에fields/buffer를저장하지않는다.
bool LoadFortContents(std::uint32_t parent,std::uint32_t stream,std::uint32_t flags,std::uint32_t relaxedConversion,const FortContentsLoadGlobals& globals,const FortContentsLoadHooks& hooks) {
    bool result{}; //선택한전체원문호출의반환값이다.
    NeedContents(hooks.access)(stream,[&](auto fields,const auto& memory) {
        std::vector<ContentsLoadFrame> frames; //원본자식스택과같은독립현재상태이며별도parsed목록은만들지않는다.
        const auto enter=[&](std::uint32_t selected,std::uint32_t attachFlags) { //각재귀진입도현재data/end게이트를다시읽는다.
            if (!ContentsU(fields,0) || !ContentsU(fields,8)) return false;
            const auto count=ContentsByte(fields,memory);frames.push_back({selected,attachFlags,count});return true;
        };
        if (!enter(parent,flags)) return;
        while (!frames.empty()) { //한항목을읽고즉시Create한뒤필요하면그자리에서자식프레임을진행한다.
            auto& frame=frames.back();if (!frame.remaining) { frames.pop_back();continue; }--frame.remaining;
            auto kind=std::uint32_t(ContentsByte(fields,memory)); //현재파일BYTE를미리parsed타입으로치환하지않는다.
            if (kind<0xf6U) {
                auto conversion=globals.conversion; //원문EAX의첫현재pointer값이며진단을거친경우에만다시읽는다.
                if (conversion) {
                    if (!relaxedConversion && NeedContents(globals.checking)() && !ContentsByteAt(NeedContents(hooks.conversionBuffer)(conversion),kind)) {
                        NeedContents(hooks.assertion)(FortContentsLoadAssertion::MissingTypeConversion);conversion=globals.conversion;
                    }
                    kind=ContentsByteAt(NeedContents(hooks.conversionBuffer)(conversion),kind);
                }
            }
            if (!kind) return;
            const auto savedFlags=NeedContents(hooks.type)(kind).flags1;std::uint32_t quantity{}; //첫TYPE플래그는뒤하위가TYPE를바꾸어도QA/QB/container판정에유지한다.
            if (savedFlags&kContentsQA) quantity=ContentsByte(fields,memory);
            if (savedFlags&kContentsQB) quantity=ContentsWord(fields,memory,hooks);
            auto listFlags=NeedContents(hooks.type)(kind).contentListFlags; //두번째TYPE조회는첫조회와구별한다.
            if (!listFlags) { listFlags=NeedContents(hooks.parentListFlags)(frame.parent);if (!listFlags) listFlags=ContentsByte(fields,memory); }
            listFlags&=~0x100U;if (listFlags&0x10U) frame.flags=(frame.flags&~8U)|0x10U;
            const auto child=NeedContents(hooks.create)(frame.parent,kind,quantity,listFlags,frame.flags,0); //원문이실제로Create한뒤에만저장된container비트를본다.
            if ((savedFlags&kContentsContainer) && !enter(child,frame.flags)) return;
        }
        result=true;
    });
    return result;
}
}
