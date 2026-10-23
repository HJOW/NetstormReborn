// 원문 직접 선택·캐시 부분 쓰기·진단 복귀 뒤 현재 포인터의 alias를 보존한다.
#include "o/RawPlayerCoreData.h"
#include <array>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 한 원문 load의 전체 폭을 확인한 뒤 같은 폭을 캐시에 써서 다음 load와 겹치는 효과도 보존한다.
void CopyCorePiece(std::span<std::uint8_t> destination,std::span<const std::uint8_t> source,std::size_t offset,std::size_t width) {
    if (offset>source.size() || width>source.size()-offset) throw std::out_of_range("현재 CoreData 원문 load 물리 범위");
    std::array<std::uint8_t,4> word{}; //원문 DWORD/WORD 레지스터에 캡처한 실제 바이트다.
    for (std::size_t byte=0;byte<width;++byte) word[byte]=source[offset+byte]; //대상 store보다 먼저 전체 source를 읽는다.
    if (offset>destination.size() || width>destination.size()-offset) throw std::out_of_range("CoreData cache 원문 store 물리 범위");
    for (std::size_t byte=0;byte<width;++byte) destination[offset+byte]=word[byte]; //앞선 store의 효과를 다음 source load에도 유지한다.
}
}
// getter의 진단을 생략한 단순 span 반환으로 치환하지 않는다.
std::span<std::uint8_t> ReadPlayerCoreData(PlayerCoreDataGlobals globals,const PlayerCoreDataHooks& hooks) {
    if (globals.metamap!=0 && globals.current.data()!=globals.cached.data()) {
        if (!hooks.assertion) throw std::logic_error("현재 CoreData 실제 Player.cpp85 진단 장치 누락");
        hooks.assertion(0x55);
    }
    return globals.current;
}
// 선택/정리는 실제 native store 순서대로 수행하며 Session 생성/파괴 자체에 암묵 호출하지 않는다.
void SelectPlayerCoreData(PlayerCoreDataGlobals globals,std::uint32_t owner,std::uint32_t templateOwner) {
    globals.templateOwner=templateOwner;globals.battleOwner=owner;
    if (owner==0) {
        const auto source=globals.current; //원문의 EAX에 캡처한 포인터는 세 load 동안 변하지 않는다.
        if (source.data()!=nullptr) { CopyCorePiece(globals.cached,source,0,4);CopyCorePiece(globals.cached,source,4,4);CopyCorePiece(globals.cached,source,8,2); }
        globals.current={};globals.active=0;return;
    }
    globals.playerKnown=1;
    if (globals.metamap!=0) { globals.current=globals.cached;return; }
    const std::uint32_t offset=owner*0xb4U+0x6cU; //원문32비트 imul/주소 덧셈이며 signed 산술 UB를 만들지 않는다.
    const auto players=globals.players; //비영/비metamap 선택의 실제 위치에서 현재 base를 한 번 읽는다.
    if (offset>players.size() || 10>players.size()-offset) throw std::out_of_range("CoreData 선택의 실제 Player 물리 부족");
    globals.current=players.subspan(offset,10);
}
}
