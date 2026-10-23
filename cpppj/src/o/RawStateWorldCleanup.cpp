// 실제State부모의현재값재조회·직접필드쓰기·선택하위호출순서를보존한다.
#include "o/RawStateWorldCleanup.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원문이실제로도달한장치만그위치에서요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("State정리의실제선택장치누락");return function; }
// 현재실제Player물리에한DWORD만쓴다.다른필드/기록을대신초기화하지않는다.
void ClearPlayerField(const StateWorldCleanupGlobals& globals,std::uint32_t player,std::size_t field) {
    auto bytes=Need(globals.players)();const auto offset=std::size_t(player)*180+field; //각원문store가현재base를다시읽는다.
    if (offset>bytes.size() || 4>bytes.size()-offset) throw std::out_of_range("State정리현재Player필드물리부족");
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=0; //같은원문DWORD의인접필드밖은보존한다.
}
}
// 전역접근장치는실제owner의순수읽기/직접쓰기로서외부callback효과를임의로추가하면안된다.
void CleanupStateWorld(std::uint32_t reason,const StateWorldCleanupGlobals& globals,const StateWorldCleanupHooks& hooks) {
    const auto read=[&](std::uint32_t address) { return Need(globals.read32)(address); }; //각사용위치의현재DWORD다.
    const auto write=[&](std::uint32_t address,std::uint32_t value) { Need(globals.write32)(address,value); }; //실제전역에즉시쓴다.
    const auto wide=[&](std::uint32_t address,std::uint64_t bits=0) { Need(globals.write64Bits)(address,bits); }; //원문double0또는마지막기한을쓴다.
    const auto effect=[&](StateWorldCleanupEffect selected) { Need(hooks.effect)(selected); }; //해당실제자식만필수다.
    using Effect=StateWorldCleanupEffect;
    if (reason==0) { effect(Effect::CloseAllConnections);effect(Effect::EndConnections); }
    else if (read(0x5c85c0)==0) { for (std::uint32_t player=1;player<9;++player) Need(hooks.disconnect)(player); } //진입분기는한번만판정한다.
    else Need(hooks.disconnect)(9);
    Need(hooks.disconnect)(10);Need(hooks.disconnect)(11);
    const auto fort=read(0x594fb8); //PE는두전역0쓰기보다먼저fort를캡처한다.
    write(0x5c85c0,0);write(0x54db14,0);
    const bool invalid=fort!=0 ? (read(0x594fbc)!=0 || read(0x594fc8)!=0) : (read(0x594fbc)!=0 && read(0x594fc8)!=0); //단일모드만허용하며읽기short-circuit순서도원문과같다.
    if (invalid) Need(hooks.assertion)(0x77a);
    while (Need(hooks.pauseDepth)()!=0) { effect(Effect::ResumeGame);effect(Effect::ResumeAudio); } //정지깊이를매반복복귀뒤재조회한다.
    effect(Effect::Clear446e70);effect(Effect::Clear4c1f20);effect(Effect::ResetClock);effect(Effect::Clear42b1b0);
    for (std::uint32_t word=0;word<22;++word) write(0x5c84e8+word*4,0); //실제22DWORD범위만rep stosd순서로0쓴다.
    auto selected=read(0x540cac);const auto manager=Need(globals.templateManager)(selected); //이두읽기는다음8개전역쓰기보다앞이다.
    write(0x5467b8,0);write(0x542544,1);write(0x5950ac,0);write(0x5950b0,0);write(0x595090,0);write(0x5c85e0,0);write(0x5c85cc,0);write(0x5c85c8,0);
    if (manager!=0) {
        if (read(0x594fb8)!=0 && read(0x5c85a4)==0) { Need(hooks.saveFort)(selected);selected=read(0x540cac); } //실제Fort저장복귀일때만owner를다시읽는다.
        if (read(0x594fbc)!=0 && read(0x594fa4)==0 && read(0x5c8758)==14) Need(hooks.saveBattle)(selected);
    }
    for (std::uint32_t player=1;player<9;++player) { //Player0은생략하고각실제DWORDstore마다현재base를읽는다.
        for (std::size_t field=0x7c;field<=0x98;field+=4) ClearPlayerField(globals,player,field); //8개전투관련필드만0쓴다.
        Need(hooks.resetAi)(player);Need(hooks.resetPriest)(player);
        if (read(0x595344)==0) Need(hooks.resetOtherPlayer)(player,0);
        else {
            const auto owner=read(0x540c70); //PE가SP호출까지보관한현재owner다.
            if (player!=owner) Need(hooks.resetOtherPlayer)(player,0);
            else { const auto bits=read(0x5c85a4)!=0?0x483d9c80U:0U;Need(hooks.setSp)(owner,std::bit_cast<float>(bits));effect(Effect::RefreshSp); }
        }
    }
    effect(Effect::ClearTemplates);const auto metamap=read(0x594fc8);write(0x54c8a0,0); //PE는metamap을0쓰기보다먼저캡처한다.
    if (metamap!=0) effect(Effect::LeaveMetamap);
    effect(Effect::Clear445140);effect(Effect::ClearCursor);
    const auto defaultSp=static_cast<float>(std::bit_cast<std::int32_t>(read(0x540bdc)));const auto type=read(0x541240); //원문은double이아닌FILD signedDWORD→float32저장이다.
    write(0x5467bc,0);write(0x59508c,0);write(0x5424bc,25);Need(globals.typeDefault)(type)=defaultSp;wide(0x5950b8);
    write(0x5453e4,0);wide(0x5950c8);wide(0x5950c0);wide(0x565e68);wide(0x5c8618);wide(0x595358);
    effect(Effect::ClearPlayerLinks);effect(Effect::ClearKernel);effect(Effect::KillGumps);effect(Effect::CleanupGumps);effect(Effect::ReplaceUber);effect(Effect::Clear43b540);
    Need(globals.write16)(0x5c89b4,0);write(0x595344,0);Need(hooks.selectCore)(0,1); //현재Player해제보다먼저실제Core10BYTE정리/NULL선택을완료한다.
    write(0x5ca8f0,0);write(0x5ca938,0);Need(hooks.resetPool)(0);
    effect(Effect::Reset422490);effect(Effect::Clear4c2450);effect(Effect::MapMode);effect(Effect::SceneMusic);effect(Effect::Clear472830);effect(Effect::Clear4b2b50);effect(Effect::Clear4328a0);effect(Effect::Clear4ab940);
    effect(Effect::Reset568b60);effect(Effect::Reset54c8a0);write(0x54c8a0,1);write(0x5c8bb0,0);effect(Effect::ResetGraph);write(0x59a8ac,0);write(0x59a8b0,1);
    effect(Effect::Clear4a9e50);effect(Effect::InitializePlayers);effect(Effect::ResetZone);
    const auto editor=read(0x5c85a4); //원문cmp결과를다음직접전역쓰기동안보관한다.
    write(0x5c85a0,0);write(0x55a420,0);write(0x5c8490,0xc61c4000);write(0x5c848c,0xc61c4000);
    if (read(0x595344)!=0) Need(hooks.setSp)(read(0x540c70),std::bit_cast<float>(editor!=0?0x483d9c80U:0U));
    effect(Effect::RefreshSp);const auto state=read(0x5c8758);const auto mission=read(0x594fa0); //State/mission은모드0쓰기앞에서캡처한다.
    write(0x595088,0);write(0x5c85d4,0);write(0x594fa4,mission);write(0x594fb8,0);write(0x594fbc,0);write(0x594fc8,0);write(0x5c85a4,0);
    if (state!=29 && state!=33) Need(hooks.selectCore)(1,1);
    effect(Effect::InterpretOptions);write(0x5949d4,read(0x5949d4)+1U);effect(Effect::MapMode);
    const auto now=Need(hooks.wallSeconds)();wide(0x5c8610,std::bit_cast<std::uint64_t>(now+10.0)); //PE의00501450은double10이며프레임시각을재사용하지않는다.
    write(0x5c7a74,0);wide(0x5c85f0);write(0x5c85b4,0);write(0x540bc0,0);write(0x540bc4,0);wide(0x5c85f8);
    write(0x542534,0xffffffff);write(0x542538,0xffffffff);write(0x54253c,0xffffffff);write(0x542540,0xffffffff);
}
}
