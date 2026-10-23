// 현재전역재조회와Create뒤고정Player주소를분리하여원문부분효과를보존한다.
#include "o/RawFortCoreWrite.h"
#include <array>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원문에서실제로선택된하위함수만필수로요구한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("CoreWrite의선택된실제장치누락");return function;
}
// 원문포인터의같은살아있는물리할당에서선택폭만반환한다.
std::span<const std::uint8_t> Physical(std::span<std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("CoreWrite의고정Player물리범위");return bytes.subspan(offset,width);
}
// unsignedBYTE/WORD를실제littleendian필드폭으로다시읽는다.
std::uint32_t Unsigned(std::span<std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    const auto field=Physical(bytes,offset,width);std::uint32_t result{};
    for (std::size_t index=0;index<field.size();++index) result|=std::uint32_t(field[index])<<(index*8);return result; //상위의다른Player바이트는읽지않는다.
}
}
// Editor자연관문은manager/Metamap진단뒤이며직렬화/RNG/SP를미리요구하지않는다.
void WriteFortCoreData(std::int32_t slot,const FortCoreWriteGlobals& globals,const FortCoreWriteHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortCoreWriteAssertion::MissingTemplate);
    if (Need(globals.metamap)()) Need(hooks.assertion)(FortCoreWriteAssertion::InMetamap);
    if (Need(globals.missionPost)()) return;
    if (Need(globals.editor)()) return;
    const auto manager=Need(hooks.manager)(slot);const auto stream=Need(hooks.create)(manager,"CoreData");
    const auto player=slot>=1 && slot<=8 ? static_cast<std::uint32_t>(slot) : 0U; //원문signed주소차이/20의정상정렬slot입력을같은Player0관문으로좁힌다.
    const auto players=Need(globals.players)();const auto offset=std::size_t(player)*180U; //Create뒤현재base를한번만보관하며callback의현재base변경으로바꾸지않는다.
    if (player!=Need(globals.playerOwner)()) Need(hooks.assertion)(FortCoreWriteAssertion::WrongPlayer);
    Need(hooks.refresh)();const auto& write=Need(hooks.write);write(stream,Physical(players,offset+0x6cU,10));
    auto& seed=Need(globals.seed)(); //앞선write복귀뒤실제고정DWORD를빌리며Editor자연ret의장치를미리요구하지않는다.
    if (static_cast<std::uint16_t>(seed)==0) {
        Need(hooks.prepareRandom)();std::uint16_t low{}; //준비callback이seed를바꿔도원문do의첫Next를생략하지않는다.
        do { //실제반환low16이비영일때까지같은난수를계속소비한다.임의횟수제한을넣지않는다.
            low=static_cast<std::uint16_t>(Need(hooks.nextRandom)(0x10000U));seed=(seed&0xffff0000U)|low;
        }while (!low);
    }
    const auto local=seed;const std::array<std::uint8_t,2> seedBytes{static_cast<std::uint8_t>(local),static_cast<std::uint8_t>(local>>8)}; //실제DWORD를로컬에보관한뒤두BYTE만일반writer로쓴다.
    Need(hooks.write)(stream,seedBytes); //앞writer가현재global을바꿔도이로컬두BYTE는바뀌지않는다.
    const auto rank=Unsigned(players,offset+0x70U,1);Need(hooks.sp)(stream,static_cast<float>(rank));
    const auto first=Unsigned(players,offset+0x6cU,2);Need(hooks.sp)(stream,static_cast<float>(first));
    const auto second=Unsigned(players,offset+0x6eU,2);Need(hooks.sp)(stream,static_cast<float>(second));
}
}
