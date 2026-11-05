// 원문 직접 선택·캐시 부분 쓰기·진단 복귀 뒤 현재 포인터의 alias를 보존한다.
#include "o/RawPlayerCoreData.h"
#include "o/RiftType.h"
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
// 원문 포인터에 해당하는 같은 물리 뒤쪽 뷰를 빌린다. 필요한 전체10BYTE를 미리 요구하지 않는다.
std::span<std::uint8_t> CoreAt(std::span<std::uint8_t> bytes,std::uint32_t offset) {
    if (offset>bytes.size()) throw std::out_of_range("CoreData 복원의 실제 시작 물리 범위");
    return bytes.subspan(offset);
}
// 선택된 원문 BYTE/DWORD load만 검사하고 현재 little endian 값을 읽는다.
std::uint32_t RestoreRead(std::span<const std::uint8_t> bytes,std::uint32_t offset,std::uint32_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("CoreData 복원의 실제 load 물리 범위");
    std::uint32_t value{};
    for (std::uint32_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8); // 동일 원문 load의 전체 폭을 캡처한다.
    return value;
}
// 원문 현재 포인터 쓰기를 host에서 표현할 수 있는 Player10BYTE의 같은 alias로 저장한다.
void RestoreCurrent(PlayerCoreDataGlobals globals,std::span<std::uint8_t> players,std::uint32_t offset) {
    if (offset>players.size() || 10>players.size()-offset) throw std::out_of_range("CoreData 복원 뒤 현재 포인터의 실제 물리 범위");
    globals.current=players.subspan(offset,10);
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
// 패치 진단 뒤 Player base를 다시 읽고 이미 선택한 local/remote 경로는 재판정하지 않는다.
void RestorePlayerCoreData(OriginalEdition edition,PlayerCoreDataGlobals globals,PlayerCoreRecordsView records,
    PlayerCoreRestoreState state,std::uint32_t owner,const PlayerCoreRestoreHooks& hooks) {
    if (state.missionPost) return;
    const bool patch=edition==OriginalEdition::Patch1078;
    const auto stride=patch ? 0xb4U : 0xacU,coreOffset=patch ? 0x6cU : 0x64U,savedOffset=coreOffset+5U;
    const auto playerOffset=owner*stride,recordOffset=records.origin+owner*16U;
    const auto initialKnown=patch ? globals.playerKnown : 0U; // PATCH는 Player base/보존BYTE보다 먼저 known을 캡처한다.
    auto players=globals.players;const auto cdOwner=patch ? 0U : globals.battleOwner; // CD는 같은 초기 base와 localOwner를 끝까지 보존한다.
    const auto saved=static_cast<std::uint8_t>(RestoreRead(players,playerOffset+savedOffset,1));
    const bool local=patch ? (initialKnown==0 || owner==globals.battleOwner) : (globals.playerKnown==0 || owner==cdOwner);
    const auto checking=patch ? state.checking.Read() : 0U; // PATCH local 경로의 active1 쓰기보다 먼저 읽은 checking이다.
    if (local) globals.active=1;
    if (checking) {
        const auto subscriber=RestoreRead(players,playerOffset+0x2cU,4);
        const auto cachedSubscriber=RestoreRead(records.bytes,recordOffset,4);
        if (subscriber!=cachedSubscriber) {
            if (!hooks.assertion) throw std::logic_error("CoreData 복원의 현재 원문 진단 장치 누락");
            hooks.assertion("getPlayer(playerId)->subscriberId == cachedCoreData[playerId].subId","\\Ns\\O\\Player.cpp",local ? 0x224U : 0x21fU);
            players=globals.players;
        }
    }
    const auto destination=CoreAt(players,playerOffset+coreOffset),source=CoreAt(records.bytes,recordOffset+4U);
    CopyCorePiece(destination,source,0,4);CopyCorePiece(destination,source,4,4);CopyCorePiece(destination,source,8,2);
    if (local) {
        if (patch) RestoreCurrent(globals,globals.players,globals.battleOwner*stride+coreOffset);
        else RestoreCurrent(globals,players,cdOwner*stride+coreOffset);
    }
    if (!state.savedGame) return;
    const auto restored=patch ? globals.players : players;const auto offset=playerOffset+savedOffset;
    if (offset>=restored.size()) throw std::out_of_range("CoreData 저장BYTE 복원의 실제 store 물리 범위");
    restored[offset]=saved;
    if (patch) {
        if (!hooks.initializeChat) throw std::logic_error("CoreData 복원의 실제 Chat 재초기화 장치 누락");
        hooks.initializeChat();
    }
}
}
