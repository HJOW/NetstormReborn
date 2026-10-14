# -*- coding: utf-8 -*-
"""기존 직접 조립 원문을 출발점으로 생산 세션의 실제 모듈 소유 코드를 준비한다."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]  # 동일 작업공간의 직접 source다.
source=(root/'cpppj/tests/CampaignInstallSupport.h').read_text(encoding='utf-8')
body=source.split('struct InstallScene {',1)[1].rsplit('};',1)[0]
body=body.replace('const netstorm::test::dudespawn::DudeAssets& assets;','SessionAssets assets;')
body=body.replace('GameRandom random{0x12345678};','GameRandom random;')
body=body.replace('SoundList soundNames{OriginalEdition::Patch1078};','SoundList& soundNames;')
body='\n    RawCampaignSessionServices services; // 실제 클라이언트의 선택 서비스이며 검사 관찰 기본값을 사용하지 않는다.\n'+body
body=body.replace('    std::unordered_set<SoundHandle> playingSounds; // 소리 장치 외부 관찰 입력의 현재 재생 토큰이다.\n','')
body=body.replace('    std::vector<std::string> external; // 실제 UI/소리/전송 요청의 검사 관찰 기록이다.\n','')
body=body.replace('piece{107},crt{7};','piece{107},crt{};')
body=body.replace('    std::int32_t noRegion{127};','    std::uint32_t randomMode{}; // 실제 재시드에서 임시로 변경하는 현재 DWORD다.\n    RawGameRandomSeed seed; // 같은 CRT/게임 난수와 실제 Unix 초 입력을 연결한다.\n    std::int32_t noRegion{127};')
body=body.replace('    explicit InstallScene(std::string_view mission="thewarbegins"):assets(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078)),metadata(assets.game),',
'''    Impl(const GameAssets& game,const BaseFileSystem& files,FortTemplate fort,std::string mission,RawCampaignSessionServices input):
        services(std::move(input)),assets(game,files),metadata(assets.game),''')
body=body.replace('sp(random,[] { return 1U; }),','sp(random,[this] { return seed.Reseed(); }),seed(random,crt,randomMode,services.unixSeconds),')
body=body.replace('hp(core.pool,core.types,core.bookkeeping,rewardState,&sp),themes','hp(core.pool,core.types,core.bookkeeping,rewardState,&sp),soundNames(RequireSounds(services)),themes')
body=body.replace('InstallForbidden','SessionMissing').replace('CHECK(','SessionCheck(')
body=body.replace('RawPriestWorldEffects priestEffects;','auto priestEffects=services.priest;')
body=body.replace('priestEffects.ownerNotice=[this](Sid) { external.push_back("사제 Owner 통지"); };','FillMissing(priestEffects.ownerNotice);')
start=body.index('        soundNames.Initialize();SoundProcessHooks soundHooks;')
end=body.index('        priestEffects.fallSound=',start)
body=body[:start]+'''        sounds=std::make_unique<SoundProcessSystem>(core.processes,soundState,std::move(services.sound));
        priestEffects.shield.loadSound=[this](std::string_view name) { return soundNames.Lookup(name); };
        priestEffects.shield.attachSound=[this](Sid parent,std::uint32_t sound,std::uint32_t alternate,std::uint32_t flags) {
            if (!sounds->Add(parent,sound,alternate,flags)) throw std::logic_error("실제 보호막 Sound47 부착 실패");
        };
        FillMissing(priestEffects.shield.noticeSound);FillMissing(priestEffects.shield.translate);FillMissing(priestEffects.shield.tell);
'''+body[end:]
body=body.replace('PlayerRegularHooks playerRegularHooks;','auto playerRegularHooks=services.player;')
body=body.replace('playerRegularHooks.log=[this](std::uint32_t,std::string_view text) { external.push_back(std::string(text)); };','FillMissing(playerRegularHooks.log);')
start=body.index('        ownership=std::make_unique<RawCampaignOwnership>')
end=body.index('        carrier=',start)
body=body[:start]+'''        auto ownerEffects=services.ownership;
        ownerEffects.otherOwner=[this](Sid sid,std::uint32_t owner) { priests->SetOwner(sid,owner); };
        FillMissing(ownerEffects.refreshProduction);FillMissing(ownerEffects.themeMode);
        ownerEffects.missingIsland=SessionMissing<void,Sid,std::int32_t,std::uint32_t>();
        ownership=std::make_unique<RawCampaignOwnership>(world,*islands,assetState,players,metadata.frames,themes,random,paint,votes,vortexRevision,97,121,std::move(ownerEffects));
'''+body[end:]
body=body.replace('RawCampaignHitEffects hitEffects;','auto hitEffects=services.hit;')
start=body.index('        // 소유자 AI/알림은')
end=body.index('        hits=',start)
body=body[:start]+'''        FillMissing(hitEffects.priest.ownerDamageNotice);
        FillMissing(hitEffects.burning.create);FillMissing(hitEffects.burning.bulfSound);
        FillMissing(hitEffects.dudes.createBurning);FillMissing(hitEffects.dudes.pathEndpoint);FillMissing(hitEffects.dudes.startPath);
'''+body[end:]
body=body.replace('RawCampaignArcherEffects combatEffects;','auto combatEffects=services.archer;')
start=body.index('        combatEffects.animation.common.sound=')
end=body.index('        combatEffects.missile.muzzle=',start)
body=body[:start]+'''        FillMissing(combatEffects.animation.common.sound);
        FillMissing(combatEffects.animation.common.targetPoint);
'''+body[end:]
start=body.index('        combatEffects.missile.firingSound=')
end=body.index('        combatEffects.missile=hits',start)
body=body[:start]+'''        FillMissing(combatEffects.missile.firingSound);FillMissing(combatEffects.missile.impactSound);
'''+body[end:]
body=body.replace('RawCampaignAssetEffects assetEffects;','auto assetEffects=services.assets;')
start=body.index('        // 사운드 핸들은 검사')
end=body.index('        assetBindings=',start)
body=body[:start]+'''        // 실제 TYPE 사운드 이름을 클라이언트의 단일 이름표에서 조회한다. TYPE번호를 토큰으로 대신하지 않는다.
        assetEffects.postPop.typeDword=[this](Sid sid,std::uint32_t offset) {
            if (offset!=0x74) throw std::logic_error("세션 TYPE 사운드 필드 미지원");
            const auto name=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition.String("impactSound");
            return name && !name->empty() ? soundNames.Lookup(*name) : 0U;
        };
        FillMissing(assetEffects.postPop.playImpactSound);FillMissing(assetEffects.queueCommand);FillMissing(assetEffects.sendGunPacket);
'''+body[end:]
start=body.index('        // 서버 Form 전송 요청은')
end=body.index('        worldHooks.regular=',start)
body=body[:start]+'''        worldHooks.formTransmit=services.dependencyTransmit;
        FillMissing(worldHooks.formTransmit);
        worldHooks.assets=services.deletion;FillMissing(worldHooks.assets.emit);FillMissing(worldHooks.assets.selected);
        worldHooks.assets=hits->DestroyHooks(std::move(worldHooks.assets));
        worldHooks.destroy=hits->Destroy(services.destroy);
'''+body[end:]
body=body.replace('contentForms=std::make_unique<ContentFormHost>(core.pool,core.types,[this](Sid,Sid,std::uint32_t) { external.push_back("내용물 부모 통지"); });',
'FillMissing(services.contentNotice);contentForms=std::make_unique<ContentFormHost>(core.pool,core.types,services.contentNotice);')
body=body.replace('templates[1]=FortTemplate::Parse(assets.files.Read("d/"+std::string(mission)+".fort"),assets.game.TypeTable());','templates[1]=std::move(fort);')
body=body.replace('RawCampaignInstallEffects hooks;','auto hooks=services.install;')
start=body.index('        hooks.translateYou=')
end=body.index('        hooks.builderAssertion=',start)
body=body[:start]+'''        FillMissing(hooks.translateYou);FillMissing(hooks.setConfig);
        FillMissing(hooks.refreshFortSpecification);FillMissing(hooks.resetCamera);
'''+body[end:]
body=body.replace('~InstallScene()','~Impl()')
body=body.replace('missileTypes[type].firingSound=firingSound && !firingSound->empty() ? type : 0;','missileTypes[type].firingSound=firingSound && !firingSound->empty() ? soundNames.Lookup(*firingSound) : 0;')
body=body.replace('missileTypes[type].impactSound=impactSound && !impactSound->empty() ? type : 0;','missileTypes[type].impactSound=impactSound && !impactSound->empty() ? soundNames.Lookup(*impactSound) : 0;')
assert 'external.' not in body and 'netstorm::test' not in body and 'CHECK(' not in body
prefix='''// 기존 원본 몸체의 단일 저장소 조립을 실제 클라이언트 세션이 소유한다.
#include "client/RawCampaignSession.h"
#include "client/PriestPlacementAssets.h"
#include "client/RawCampaignGunQueries.h"
#include "o/RawGameRandomSeed.h"
#include "o/RawSquidFinder.h"
#include "o/RawSquidLevel.h"
#include "o/RawPlayerTemplateState.h"
#include "o/RawArcherTypeData.h"
#include "o/RawProjectileGeometry.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::o;
namespace netstorm::client {
namespace {
// 아직 미공급인 실제 하위는 선택 시 구체적 함수 인자형을 포함한 오류를 반환한다.
template<class Result,class... Args> std::function<Result(Args...)> SessionMissing() {
    return [](Args...)->Result { throw std::logic_error(std::string("실제 세션 하위 소비자 미연결: ")+__FUNCSIG__); };
}
// optional 경계를 성공으로 채우지 않고 실제 선택 시 요청을 보고한다.
template<class Result,class... Args> void FillMissing(std::function<Result(Args...)>& callback) { if (!callback) callback=SessionMissing<Result,Args...>(); }
// 동일 풀의 실제 예약/부착 실패를 즉시 보고한다.
void SessionCheck(bool value) { if (!value) throw std::logic_error("실제 캠페인 세션 예약/자료 조건 오류"); }
// 하나의 실제 클라이언트 소리 이름표만 사용한다.
SoundList& RequireSounds(const RawCampaignSessionServices& services) { if (!services.soundNames) throw std::invalid_argument("세션의 실제 SoundList 누락");return *services.soundNames; }
// 현재 원본 초기 다리/받침/미션 전역이다.
BridgeConnectState CampaignLinks() { BridgeConnectState result;result.bridgeType=82;result.islandType=94;result.stalagType=95;result.connectorType=155;result.noIslandType=157;result.battle=true;result.mission=true;return result; }
// 보호 GameAssets의 실제 TYPE/프레임을 보관하며 검사 전용74 변경을 적용하지 않는다.
struct SessionAssets {
    const GameAssets& game; // 클라이언트가 소유하는 단일 자산이다.
    const BaseFileSystem& files; // 동일 보호 VFS 참조다.
    std::vector<RiftTypeRecord> types; // 실제 원본 전체 TYPE 자료다.
    std::vector<RiftTypeFrames> frames; // 같은 물리 블록의 실제 프레임 Run이다.
    // PATCH 판본과 actual 자료를 첫 모듈 생성 전에 확인한다.
    SessionAssets(const GameAssets& source,const BaseFileSystem& storage):game(source),files(storage),types(source.TypeTable().Types().begin(),source.TypeTable().Types().end()),frames(types.size(),RiftTypeFrames({})) {
        if (source.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("실제 세션의 CD판 조립 미연결");
        // 실제 로딩 번호70부터 프레임 표를 직접 채운다.
        for (const auto& asset:source.Types()) frames[70+asset.block]=asset.definition.FrameTable();
    }
};
// 원본 비정렬 DWORD를 지정 raw 필드에 쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) { for (std::size_t i=0;i<4;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); }
}
// 실제 월드/상태와 모든 callback 대상을 같은 고정 주소에 소유한다.
struct RawCampaignSession::Impl {'''
suffix='''
};
// 현재 실제 자산/Template/서비스를 보관하고 모든 하위 모듈을 구성한다.
RawCampaignSession::RawCampaignSession(const GameAssets& assets,const BaseFileSystem& files,FortTemplate fort,std::string mission,RawCampaignSessionServices services)
    :impl_(std::make_unique<Impl>(assets,files,std::move(fort),std::move(mission),std::move(services))) {}
// Impl이 Kernel을 비운 뒤 실제 모듈을 해제한다.
RawCampaignSession::~RawCampaignSession()=default;
// 동일 원본 부모를 실행한다.
void RawCampaignSession::Load() { impl_->runtime->Load(); }
// 현재 같은 시각을 공격/소리/실제 Kernel에 공급한다.
void RawCampaignSession::RunFrame(FrameTime time,bool paused) { impl_->assetBindings->PrepareFrame(time.game);impl_->combat->PrepareFrame(time);impl_->soundState.frame=static_cast<std::uint32_t>(time.number);impl_->world.RunFrame(time,paused); }
// 실제 이동 조립을 연결하기 전 명령을 성공으로 대신하지 않는다.
bool RawCampaignSession::MovePriest(Sid,RawPathPoint) { throw std::logic_error("실제 세션 경로 조립 진행 중"); }
// 화면과 입력은 같은 살아 있는 상태를 읽는다.
RawSurfaceWorld& RawCampaignSession::World() { return impl_->world; }
RawWorldCore RawCampaignSession::Core() { return impl_->core; }
const SquidReward& RawCampaignSession::HitPoints() const { return impl_->hp; }
const RawPriestWorldBindings& RawCampaignSession::Priests() const { return *impl_->priests; }
}
'''
(root/'cpppj/src/client/RawCampaignSession.cpp').write_text(prefix+body+suffix,encoding='utf-8',newline='\n')
print('생산 세션 준비 원문:',len((prefix+body+suffix).splitlines()),'행')
