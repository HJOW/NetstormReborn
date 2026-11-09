// 오벨리스크보상의실제Deck/현재장치/같은물리를Client수명에서공유한다.
#include "client/ClientConstructionGlobals.h"
#include "client/ClientRandomGlobals.h"
#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>

namespace netstorm::client { //현재전역을다시조회하며임시World/Player/seed를추가하지않는다.
// 현재메뉴/건설과동일목록의주소를반환하고재결합에서지우지않는다.
o::SquidPostPopList& ClientConstructionGlobals::ConstructionExcludedTypes() { static_cast<void>(State());return constructionExcludedTypes_; }
// 같은현재장치의선택호출을끝까지보유하고실제Pool의생성/Pop/가상삭제를직접사용한다.
o::RawGemReward& ClientConstructionGlobals::GemReward() {
    if (!storage_ || !random_) throw std::logic_error("오벨리스크 보상의 실제 장기 물리/난수 미결합");
    if (!gemReward_) {
        o::GemRewardHooks hooks; //앞선원문효과뒤현재장치를선택하며Session의서비스사본을보관하지않는다.
        hooks.fill=[this](auto bytes) { const auto current=devices_;if (!current || !current->allocationBytes) throw std::logic_error("오벨리스크 Deck의 실제 현재 할당 입력 미결합");const auto fill=current->allocationBytes;fill(bytes); };
        hooks.deck.wallSeconds=[this] { const auto current=devices_;if (!current || !current->wallSeconds) throw std::logic_error("오벨리스크 Deck의 실제 현재 시각 미결합");const auto wall=current->wallSeconds;return wall(); };
        hooks.assertion=[this](auto line) { const auto expression=line==0xf6 ? "0":line==0x104 ? "type":"buriedSquid.getType() == buriedType && !buriedSquid.isFree()";storage_->AssertDisplay(expression,"\\Ns\\O\\Gem.cpp",line); }; //보호PE의실제세문구와진단뒤정상복귀를보존한다.
        hooks.deck.assertion=[this](auto line) { storage_->AssertDisplay("isOpen()","\\Ns\\O\\Deck.cpp",line); };
        hooks.deck.mismatch=[](auto stored,auto current) { std::clog<<"["<<0x1d<<"] Deck chance mismatch. "<<std::bit_cast<std::int32_t>(stored)<<"!="<<std::bit_cast<std::int32_t>(current)<<"\n"; }; //기존Player그룹1d의같은현재로그스트림이다.
        hooks.deckAssertion=[this](o::ProductionDeckAssertion where) {
            constexpr std::array<std::string_view,6> expressions{"entry[i].type != type","totalEntries < MAX_ENTRIES","type==_type","chance==_chance","power==_power","numRemaining==_numRemaining"}; //44f290/44f160의실제진단순서다.
            constexpr std::array<std::uint32_t,6> lines{0x64,0x66,0x2d,0x2e,0x2f,0x30}; //동일Deck.cpp의원문행번호다.
            const auto index=static_cast<std::size_t>(where);storage_->AssertDisplay(expressions.at(index),"\\Ns\\O\\Deck.cpp",lines.at(index));
        };
        hooks.create=[this](auto type,auto flags) { return storage_->CreateAsset(type,flags); };
        hooks.pop=[this](auto sid,auto x,auto y,auto flags) { static_cast<void>(storage_->PopAsset(sid,x,y,flags)); };
        hooks.destroy=[this](auto sid,auto flags) { static_cast<void>(storage_->DestroyAsset(sid,flags)); };
        gemReward_=std::make_unique<o::RawGemReward>(storage_->pool,storage_->types,random_->Game(),constructionExcludedTypes_,gemState_,std::move(hooks));
    }
    return *gemReward_;
}
}
