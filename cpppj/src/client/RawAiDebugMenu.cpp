// 원문현재Player재조회와파라미터단계의보관brain을구별하며부분실패를되돌리지않는다.
#include "client/RawAiDebugMenu.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 현재메뉴작성기와생존게임물리만빌린다.
namespace { // 도달한하위경계/물리만검사한다.
template<class F> const F& NeedAiMenu(const F& hook) { if (!hook) throw std::logic_error("AI 메뉴 실제 하위 미연결");return hook; } //빈메뉴성공으로효과를누락하지않는다.
std::uint32_t AiMenuWord(std::span<const std::uint8_t> bytes,std::size_t offset) { // 원문비정렬DWORD를같은폭으로읽는다.
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("AI 메뉴 현재 DWORD 물리범위");std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(8*index); // 낮은BYTE부터조립한다.
    return value;
}
}
// checking과현재brain의단락뒤원문메뉴효과를원래순서대로작성한다.
void OpenAiDebugMenu(std::uint32_t owner,const AiDebugMenuReads& reads,const AiDebugMenuHooks& hooks) {
    if (!NeedAiMenu(reads.checking)() || std::bit_cast<std::int32_t>(owner)<=0) return;
    if (owner>=9) throw std::out_of_range("AI 메뉴 Player owner 물리범위");
    const auto player=NeedAiMenu(reads.player)(owner); //원문은처음계산한Player행주소를callback동안보관한다.
    const auto brain=[&] { return AiMenuWord(player,0xb0); }; // 같은행의현재+b0만효과callback뒤다시읽는다.
    if (!brain()) return;
    const auto field=[&](std::uint32_t token,std::size_t offset) { if (!token) throw std::out_of_range("AI 메뉴 재조회 NULL brain");return AiMenuWord(NeedAiMenu(reads.body)(token),offset); }; //해제/NULL을가짜본문으로대체하지않는다.
    const auto print=[&](std::string_view pattern,const PriestMenuArgument& value) { return NeedAiMenu(hooks.format)(80,pattern,{&value,1}); }; //첫공통서식요청폭80을보존한다.
    if (!field(brain(),0x258)) NeedAiMenu(hooks.header)("NOT INITIALIZED\n"); //decimal600은초기화258이며bridge의hex600이아니다.
    const auto money=print("~kMoney=%d~.\n",std::bit_cast<std::int32_t>(field(brain(),0x2d8)));NeedAiMenu(hooks.header)(money);
    constexpr std::array<std::string_view,5> goals{"None","Player","Geyser","Attack","Hookup"}; //원문목표0..4의실제이름표다.
    const auto goal=field(brain(),0x2d4);if (goal>=goals.size()) throw std::out_of_range("AI 메뉴 목표 문자열표 범위");
    const auto text=print("~kGoal=%s~.\n",std::string(goals[goal]));NeedAiMenu(hooks.header)(text);
    NeedAiMenu(hooks.checked)(field(brain(),0xb00),"AI Reporting",NeedAiMenu(reads.command)(0x542a34),owner,1,1);
    NeedAiMenu(hooks.begin)(0);
    constexpr std::array<std::pair<std::string_view,std::uint32_t>,7> diagnostics{{{"Debug Bridge",0x52f3cc},{"Debug Geyser",0x52f3e4},{"Debug Lay Piece",0x52f3d0},{"Debug Build",0x52f3d4},{"Debug Move",0x52f3d8},{"Debug Dry Spot",0x52f3dc},{"Debug Factory",0x52f3e0}}}; //실제메뉴순서다.
    for (const auto& [label,address]:diagnostics) { // 같은현재로그값을각행바로앞에서조회한다.
        const auto command=NeedAiMenu(reads.command)(0x542a38);NeedAiMenu(hooks.checked)(NeedAiMenu(reads.debug)(address)==29,label,command,address,1,1);
    }
    const auto debug=NeedAiMenu(hooks.take)();NeedAiMenu(hooks.submenu)("AI Debug",debug,1,0);
    const auto captured=brain(); //원문은AI Debug작성뒤한번읽은본문을모든파라미터행에서보관한다.
    NeedAiMenu(hooks.begin)(0);NeedAiMenu(hooks.begin)(0);
    const auto parameter=[&](std::string_view label,std::size_t offset,std::uint32_t expected,std::uint32_t packed,bool zeroFloat=false) { //공통행도호출시점의보관본문값을다시읽는다.
        const auto value=field(captured,offset);const auto checked=zeroFloat ? std::bit_cast<float>(value)==0.0F : value==expected;
        NeedAiMenu(hooks.checked)(checked,label,NeedAiMenu(reads.command)(0x5428d8),owner|packed,1,1);
    };
    const auto finish=[&](std::string_view label) { const auto token=NeedAiMenu(hooks.take)();NeedAiMenu(hooks.submenu)(label,token,1,0); }; //현재하위메뉴를한번만꺼내바로부착한다.
    parameter("0",0x220,0,0,true);parameter("1",0x220,0x3f800000,0x10000);parameter("2",0x220,0x40000000,0x20000);parameter("6",0x220,0x40c00000,0x60000);finish("aiBridgeDrawRate");
    NeedAiMenu(hooks.begin)(0);parameter("2",0x228,0x40000000,0x20010);parameter("3",0x228,0x40400000,0x30010);parameter("4",0x228,0x40800000,0x40010);finish("aiTimeBetweenMoves");
    NeedAiMenu(hooks.begin)(0);parameter("3",0x230,3,0x30020);parameter("6",0x230,6,0x60020);parameter("9",0x230,9,0x90020);finish("aiGeyserAttachments");
    NeedAiMenu(hooks.begin)(0);parameter("3",0x234,3,0x30030);parameter("6",0x234,6,0x60030);parameter("10",0x234,10,0xa0030);finish("aiCollectors");
    NeedAiMenu(hooks.begin)(0);parameter("0",0x24c,0,0x40);parameter("8",0x24c,8,0x80040);parameter("30",0x24c,30,0x1e0040);finish("aiMoneyRechargeRate");finish("AI Parameters");
}
}
