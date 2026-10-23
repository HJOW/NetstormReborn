// 원문stack잔여/현재전역/한레코드의고정Player와정상·예외해제를구별한다.
#include "o/RawSavedStateRead.h"
#include <bit>
#include <stdexcept>
#include <type_traits>

namespace netstorm::o {
namespace {
static_assert(std::is_same_v<std::uint8_t,unsigned char> && std::endian::native==std::endian::little); //같은DWORD참조의unsignedchar표현을직접파일read에빌리는지원호스트계약이다.
// 실제선택된하위만필수로요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("저장상태reader의실제선택장치누락");return function; }
// 선택한원문접근폭만검사하며짧은파일read를별도정책으로거부하지않는다.
std::span<std::uint8_t> Physical(std::span<std::uint8_t> bytes,std::size_t at,std::size_t width) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("저장상태reader의명시물리범위");return bytes.subspan(at,width);
}
// 원문littleendian의현재지역DWORD를읽는다.
std::uint32_t Get(std::span<std::uint8_t> bytes,std::size_t at) {
    const auto field=Physical(bytes,at,4);std::uint32_t result{};
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(field[index])<<(index*8);return result; //이웃필드의초기stack값은읽지않는다.
}
// 지역/Player의실제지정DWORD만쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    const auto field=Physical(bytes,at,4);
    for (std::size_t index=0;index<4;++index) field[index]=static_cast<std::uint8_t>(value>>(index*8)); //부분전역과나머지Player는그대로둔다.
}
// 실제고정DWORD의unsignedchar표현을사본없이직접read에빌린다.
std::span<std::uint8_t> Dword(std::uint32_t& value) { return {reinterpret_cast<std::uint8_t*>(&value),4}; }
// 실제NUL까지의현재문자열뷰이며물리끝밖검색은host계약오류다.
std::string_view String(std::span<const std::uint8_t> bytes) {
    for (std::size_t index=0;index<bytes.size();++index) { //원문char포인터의첫NUL위치만따른다.
        if (!bytes[index]) return {reinterpret_cast<const char*>(bytes.data()),index};
    }throw std::out_of_range("저장상태reader의NUL없는물리문자열");
}
// strchr('.')처럼점또는NUL을먼저만난위치에서멈춘다.
bool Dot(std::span<const std::uint8_t> bytes) {
    for (const auto byte:bytes) { //점뒤NUL이없어도원문은즉시실패분기로갈수있다.
        if (byte=='.') return true;if (!byte) return false;
    }throw std::out_of_range("저장상태reader의점검색물리끝");
}
// NUL포함복사는한BYTE씩실행하여물리실패전까지의부분쓰기를보존한다.
void Copy(std::span<std::uint8_t> destination,std::size_t output,std::span<const std::uint8_t> source,std::size_t input) {
    for (;;) { //원문처럼읽기→한BYTE쓰기→NUL판정순서다.
        if (input>=source.size()) throw std::out_of_range("저장상태reader의문자열읽기물리끝");const auto value=source[input++];if (output>=destination.size()) throw std::out_of_range("저장상태reader의문자열쓰기물리끝");destination[output++]=value;if (!value) return;
    }
}
}
// read반환길이를사용하지않고실제현재필드에남은명시stack/전역입력을계속쓴다.
std::int32_t ReadSavedState(std::span<std::uint8_t> scratch,const SavedStateReadGlobals& globals,const SavedStateReadHooks& hooks) {
    const auto path=Need(hooks.path)();if (!Need(hooks.exists)(String(path))) return 0;
    Put(scratch,0x1c,0);Put(scratch,0x20,0);const auto object=Physical(scratch,0x0c,24); //원문은파일생성자앞에두마지막DWORD만0으로쓴다.
    const auto file=Need(hooks.open)(String(path),object,0,0);bool active=true; //생성자정상복귀뒤에만SEH파일해제의책임이생긴다.
    const auto close=[&] { active=false;Need(hooks.close)(file,object); }; //해제전에원문state를-1로바꾸는것처럼중복해제를막는다.
    const auto read=[&](std::span<std::uint8_t> target) { static_cast<void>(Need(hooks.read)(file,target,0xffffffffU)); }; //요청폭은실제span길이이며부족반환을0패딩/실패변환하지않는다.
    try {
        read(Physical(scratch,0x40,4));if (Get(scratch,0x40)!=0xa004eU) { close();return 0; }
        read(Physical(scratch,0x88,256));const auto fort=Physical(scratch,0x88,scratch.size()-0x88);if (Dot(fort)) { close();return 0; }
        Need(hooks.pending)(String(fort));auto& restoreOwner=Need(globals.owner)();restoreOwner=0xffffffffU;read(Dword(restoreOwner));read(Physical(scratch,0x3c,4));read(Physical(scratch,0x44,4));
        std::uint32_t participants{};Put(scratch,0x30,8); //원문은길이나내용에따른조기레코드종료없이8회반복한다.
        do { //매반복같은지역초기화/일곱read뒤현재Playerbase를조회한다.
            Need(hooks.initializeCore)(Physical(scratch,0x24,10));read(Physical(scratch,4,4));read(Physical(scratch,8,4));read(Physical(scratch,0x48,30));read(Physical(scratch,0x68,30));read(Physical(scratch,0,4));read(Physical(scratch,0x38,4));read(Physical(scratch,0x24,10));
            const auto number=Get(scratch,4),flags=Get(scratch,0);const auto players=Need(globals.players)();const auto offset=std::size_t(number)*180U;const auto identity=Get(scratch,8);Put(scratch,0,flags&0xffffffbfU);Put(players,offset+0x2c,identity);
            if (Get(scratch,4)==Need(globals.owner)()) { Need(globals.subscriber)()=Get(scratch,8);Copy(Need(globals.previousHandle)(),0,players,offset+0x0c); } //새이름전같은이전Player이름을보존한다.
            Copy(players,offset+0x0c,scratch,0x48);Copy(players,offset+0x30,scratch,0x68);Put(players,offset+0x50,Get(scratch,0));Put(players,offset+0x58,Get(scratch,0x38));Put(players,offset+0x6c,Get(scratch,0x24));Put(players,offset+0x70,Get(scratch,0x28));
            const auto coreTail=Physical(scratch,0x2c,2);auto playerTail=Physical(players,offset+0x74,2);playerTail[0]=coreTail[0];playerTail[1]=coreTail[1]; //원문Core는4/4/2로쓰며2BYTEpadding은포함하지않는다.
            std::array<std::uint8_t,10> core{};const auto source=Physical(scratch,0x24,10);
            for (std::size_t index=0;index<core.size();++index) core[index]=source[index]; //원문인자구조체의같은10BYTE값복제다.
            Need(hooks.metadata)(Get(scratch,4),Get(scratch,8),core);const auto currentFlags=Get(scratch,0);if ((currentFlags&1U) && !(currentFlags&0x200U)) ++participants; //메타데이터callback뒤지역flags를다시읽는다.
            Put(scratch,0x30,Get(scratch,0x30)-1U);
        }while (Get(scratch,0x30));
        for (const auto& current:globals.tail) read(Dword(Need(current)())); //같은세전역의부족읽기상위BYTE도보존한다.
        Put(scratch,0x34,0);read(Physical(scratch,0x34,4));if (Get(scratch,0x34)) { close();return 0; }
        const auto result=participants>=2 ? Get(scratch,0x3c) : 0U;close();return std::bit_cast<std::int32_t>(result); //close가저장시각을바꿔도원문처럼미리보관한결과를반환한다.
    }catch (...) { if (active) close();throw; } //실제004fb5d0의선택된파일해제뒤원래부분중단을전파한다.
}
}
