// 실제 공통 자산/기술 ContentForm/Player 부모 통지를 같은 세션 풀과 삭제 분배기에 연결한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "client/GumpEvents.h"
#include "o/ContentFormLifetime.h"
#include "o/RawPlayerContentNotice.h"

namespace netstorm::client {
// 현재 실제 등록표와 직접 가상+28소비자를 연결한다. 트리 재귀/생산Refresh 사건으로 대체하지 않는다.
o::PlayerContentNoticeHooks BindPlayerContentNoticeGumps(GumpCreateState& registry,GumpEventHooks events,o::PlayerContentNoticeHooks hooks);
// 실제공통삭제의선택/소리요청을현재UI/파생송신소비자로변환한다. Refund는실제reward가직접처리한다.
o::SquidDeletionHooks MakeCampaignDeletionEffects(o::SquidDestroyHooks effects,std::function<void(o::Sid)> unitLost);
// 기존내용물몸체만소유하고새보상/Player/공통삭제state사본을만들지않는다.
class RawCampaignLifetimeBindings {
public:
    // world와현재Content상태/Host/reward/Player통지는이객체보다오래살아야한다.
    // 실제공통후설치는world.BindDeletion으로먼저공급하며Content등록은이생성자가한번수행한다.
    RawCampaignLifetimeBindings(RawSurfaceWorld& world,o::ContentFormHost& forms,const o::ContentLifetimeState& state,
        o::ContentLifetimeHooks effects,o::SquidReward& reward,const o::RawPlayerContentNotice& player,o::ContentFormNotice fallbackParent={});
    RawCampaignLifetimeBindings(const RawCampaignLifetimeBindings&)=delete; // 분배기가빌리는동일callback주소를보존한다.
    RawCampaignLifetimeBindings& operator=(const RawCampaignLifetimeBindings&)=delete; // 다른세계와내용물수명을복사하지않는다.
    // 84/baseVT만입증된공통Pre/Post로분배하고기존다른실제자산효과를유지한다.
    o::SquidDestroyHooks DestroyHooks(o::SquidDestroyHooks fallback) const;
    // 현재84/baseVT의실제+10을기존공통삭제기로실행하며다른파생은기존분배를유지한다.
    o::SquidAssetDestroy Destroy(o::SquidAssetDestroy fallback={}) const;
    // RawContentParentNotice의실제파생callback으로사용한다.Player만새몸체에보내고다른부모는명시경계다.
    void ParentNotice(o::Sid parent,o::Sid form,std::uint32_t reason) const;
    // 현재같은내용물수명/풀의검증경계다. Session은해제전에World.ClearProcesses를실행한다.
    const o::ContentFormLifetime& Contents() const;
    const o::SidPool& Pool() const;
private:
    // 원본현재raw타입과VT둘다확인하며일반base자산전체로확장하지않는다.
    bool EmptyGeyser(o::Sid sid) const;
    RawWorldCore core_; // 실제풀/장부/공통삭제/분배기의참조만보관한다.
    const o::RawPlayerContentNotice& player_; // 같은풀의현재Player+24실제몸체다.
    o::ContentFormNotice fallbackParent_; // 워커/다이스/워크샾등다른실제부모통지경계다.
    o::ContentFormLifetime contents_; // 기존Form6의실제pre/Unpop/부모알림/post/SID반납을등록한다.
};
}
