// 같은 현재 전역을 원문 조회 위치에서 읽고 메뉴/참가자 상태를 별도 복사하지 않는다.
#include "client/ClientMetaPlayerMenu.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/ClientChallengeGlobals.h"
#include "client/ClientChatWindowGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/RawNsInfoContext.h"
#include "client/RawMenuControls.h"
#include "client/RawPlayerMenu.h"
#include "client/NativeMetaPlayerMenu.h"
#include "root/AddressCodec.h"
#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <utility> //실제현재getter의소유callback을식별자/메뉴에이동한다.

namespace netstorm::client { //Client/메뉴는반환된동기callback보다오래살아야한다.
// 원문20BYTE주소지역값은이메뉴에서소비하지않는다.공통문법/로컬이름장치의실제해석은수행한다.
void ParsePlayerMenuAddress(std::string_view text,std::uint32_t port) { root::AddressCodec address;static_cast<void>(address.Parse(std::string(text),std::bit_cast<std::int32_t>(port))); }
// CS와Session은바인딩시미리조회하지않고선택된원문분기에서만읽는다.
void BindClientMetaPlayerMenu(RawNsInfoContext& context,Client& client,RawMenuControls& menus) {
    if (!context.storage || context.storage!=client.SurfaceStorage() || context.gameplay!=client.GameplayGlobals() || context.challenge!=client.ChallengeGlobals()) throw std::invalid_argument("메타 Player 메뉴의 실제 Client owner 불일치");
    context.identity=[&client,surface=context.storage] {
        o::PlayerIdentityReads reads;reads.localOwner=[&client] { return client.GameplayGlobals()->player.battleOwner; };reads.subscriber=[&client] { return client.CurrentSubscriber(); };reads.address=[&client] { return client.GameplayGlobals()->LocalAddress(); };
        reads.name=[&client]()->std::string_view { const auto& bytes=client.SubscriberGlobals()->Handle();const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("메타 Player 메뉴의 실제 현재 handle NUL 없음");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())}; }; //현재PTR00540ca8의실제64BYTE장치를빌린다.
        reads.battle=[surface] { return surface->ownerMode.battle; };reads.editor=[&client] { return client.GameplayGlobals()->world.editor!=0; };reads.metamap=[surface] { return surface->ownerMode.challenge; };reads.unknownName=[] { return std::string_view("SOMEONE"); }; //00540bd0의보호PE문자열이며원문두getter만읽고활성writer는없다.
        reads.metaRecords=[&client]()->std::span<const std::uint8_t> { return client.ChallengeGlobals()->Bytes().subspan(o::kChallengeParticipantOffset,o::kChallengeParticipantCount*o::kChallengeParticipantBytes); };
        return o::RawPlayerIdentity(client.Edition(),client.CoreDataGlobals()->Players(),{},client.ChatRecords(),std::move(reads));
    };
    context.playerMenuHooks=[&client,&menus,surface=context.storage,identity=context.identity](PlayerMenuHooks& hooks) {
        hooks.mission=[&client] { return client.GameplayGlobals()->world.missionPost!=0; };hooks.fort=[&client] { return client.GameplayGlobals()->RequireSession().Core().bridgeState.mission; };hooks.money=[&client](auto owner) { return client.GameplayGlobals()->RequireSession().Sp(owner); }; //메뉴fort는594fa0이며inFortMode594fb8과구별한다.
        hooks.allowSend=[&client] { return client.GameplayGlobals()->RequireSession().BattleOption(13)!=0; };hooks.allowOffer=[&client] { return client.GameplayGlobals()->RequireSession().BattleOption(11)!=0; };hooks.allowBreak=[&client] { return client.GameplayGlobals()->RequireSession().BattleOption(12)!=0; };
        hooks.parseAddress=ParsePlayerMenuAddress;hooks.reliability=[&client,surface](auto index) { return MetaPlayerReliability(surface->ownerMode.challenge,index,[&client](auto participant)->std::span<const std::uint8_t> { return client.ChallengeGlobals()->Participant(participant); }); };
        hooks.ignored=[&client](auto subscriber) { const auto& list=client.ChatWindowGlobals()->ignored;return std::find(list.begin(),list.end(),subscriber)!=list.end(); };
        hooks.whispered=[&menus,identity](auto subscriber)->std::uint8_t { const auto finder=identity();o::PlayerIdentityRecord record;o::RawPlayerIdentity::Initialize(record,0);for (std::size_t byte=0;byte<4;++byte) record[4+byte]=static_cast<std::uint8_t>(subscriber>>(8*byte));if (!finder.Resolve(record)) return 0;return menus.ChatContainsRecipient({reinterpret_cast<const char*>(record.data()+8)}); }; //subscriberDWORD를낮은BYTE부터쓴현재식별자뒤에만실제Console접두를조회한다.가짜빈입력을만들지않는다.
        hooks.assertion=[](auto assertion) { std::clog<<"메타 Player 메뉴 원문 진단:"<<static_cast<int>(assertion)<<'\n'; };hooks.print=FormatPlayerMenuText;
    };
}
}
