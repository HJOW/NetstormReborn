// Session이 없는 메뉴 HUD도 원문 Deck getter와 내용물 순회 전체를 실행한다.
#include "client/ClientProductionSync.h"
#include "client/CurrentAllocationBytes.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h"
#include "o/RawProductionSync.h"
#include <cstdio>
#include <stdexcept>

namespace netstorm::client { // 실제 Client 전역/물리와 기존 원문 몸체를 값 사본 없이 조립한다.
namespace { // 현재 Player 기록의 필요한 DWORD만 원문 시점에 조회한다.
std::uint32_t PlayerField(const ProductionControlContext& context,std::uint32_t owner,std::size_t offset) {
    const auto records=context.players->Records();const auto at=std::size_t(owner)*180+offset;
    if (owner>=9 || offset>176 || at>records.size() || 4>records.size()-at) throw std::out_of_range("생산의 실제 현재 Player DWORD 없음");
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(records[at+byte])<<(byte*8);return value; //인접 필드/초기값을 섞지 않는다.
}
}
// 현재 Runtime이 있으면 기존 설치 guard를 그대로 통과해야 한다. 부재 때만 실제 장기 메모리에 조립한다.
void SynchronizeProductionControlContents(const ProductionControlContext& context) {
    if (!context.storage || !context.gameplay || !context.players) throw std::invalid_argument("생산 동기화의 실제 Client 문맥 미연결");
    if (context.gameplay->Borrowed() || context.storage->Borrowed()) { context.gameplay->RequireSession().SynchronizeProductionContents();return; }
    auto& game=*context.gameplay;auto& storage=*context.storage;auto& memory=*context.players->ProductionDeckMemory(); //현재 getter의 실제 owner 세 개를 같은 동기 호출 동안 빌린다.
    o::CarrierPreDestroyHooks carrierHooks; //조회 몸체만 선택하며 삭제 가상 경계는 실제 Runtime 없이는 성공하지 않는다.
    carrierHooks.validateDamageable=[&game](auto,auto) { static_cast<void>(game.RequireSession()); };
    carrierHooks.damageablePre=[&game](auto,auto) { static_cast<void>(game.RequireSession()); };
    carrierHooks.dependentFound=[&game] { static_cast<void>(game.RequireSession()); };
    const o::RawCarrierPreDestroy carrier(storage.pool,game.finder,std::move(carrierHooks)); //004ac9f0은 같은 원문 finder/WORD 조건 몸체다.
    o::ProductionSyncHooks hooks; //실제43df80이 도달한 자식만 현재 물리/Runtime으로 연결한다.
    hooks.deck=[&context,&game,&memory] { return std::span<const std::uint8_t>(memory.Bytes(memory.Get(context.players->Records(),game.player.templateOwner,CurrentAllocationBytes))); };
    hooks.technology=[&context,&game] { return game.player.playerKnown ? PlayerField(context,game.player.battleOwner,0x9c) : 0U; };
    hooks.temple=[&context](auto owner) { return PlayerField(context,owner,0x7c)!=0; };
    hooks.cancelCursor=[&game] { game.RequireSession().Cursor().Cancel(); };
    hooks.destroy=[&game](auto sid,auto flags) { game.RequireSession().World().Destroy(sid,flags); };
    hooks.findKind=[&carrier,&storage](auto parent,auto mask,auto kind,auto index) { const auto found=carrier.FindContained(parent,mask,kind,index);if (!found.value) return 0U;const auto raw=storage.pool.Slot(found);return std::uint32_t(raw[18])|(std::uint32_t(raw[19])<<8); };
    hooks.create=[&game](auto parent,auto kind,auto quantity,auto flags,auto attach,auto quality) { return game.RequireSession().CreateContained(parent,kind,quantity,flags,attach,quality); };
    hooks.assertion=[](std::string_view text) { std::fprintf(stderr,"생산 동기화 원본 진단: %.*s\n",static_cast<int>(text.size()),text.data()); };
    const o::RawProductionSync sync(storage.pool,storage.types,game.finder,{game.player.battleOwner,game.FullDeck()},std::move(hooks));sync.Synchronize(); //known0의 SID0/빈 Deck도 생략 없이 실제 본체가 자연 반환한다.
}
}
