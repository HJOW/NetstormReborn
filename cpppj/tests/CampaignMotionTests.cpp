// 실제 저장 캠페인의 사제 명령을 같은 Pool/Host54/BFS/도착/cleanup에서 실행한다.
#include "CampaignInstallSupport.h"
#include "client/RawCampaignMotionBindings.h"
#include <cmath>

namespace {
// 현재 공유 시계/원본 타입/명령/도착 상태를 단일 설치 장면에 연결한다.
struct MotionScene {
    InstallScene install; // 실제 보호 캠페인의 전체 초기 월드다.
    PlayerPlacementAnchorState anchorState; // 실제 Graph getter의 현재 mutable 상태다.
    std::unique_ptr<RawPlayerPlacementAnchor> anchor; // 같은 Core hash0/장부를 읽는 실제 getter다.
    RawCampaignMotionState state; // 실제 Halt/Arrival/cleanup/BFS 전역의 소유자다.
    std::uint32_t geyserType{122}; // 현재 원본 간헐천 타입 DWORD다.
    bool snap{true},retrying{}; // 원본 사제 목적지 스냅과 실패 재귀 guard다.
    RawPathStartState start; // 같은 Host clock과 사제 월드의 이동 타입을 직접 참조한다.
    RawPathProcessState process; // 같은 Kernel clock의 actual54 과정 상태다.
    std::unique_ptr<RawCampaignMotionBindings> motion; // 큰 외부 하위를 명시한 실제 실행 연결이다.
    RawPathDataHeader header; // 실제 Path Form writer의 현재 외부 header다.
    std::array<std::uint8_t,16384> packet{}; // 실제 writer가 직접 쓸 동일 버퍼다.
    std::size_t steps{},transmissions{}; // 검사 외부 음향/전송 요청의 관찰 수다.

    // 실제 미션 파일을 먼저 설치한 뒤 동일 사제의 현재 raw 명령 경로를 구성한다.
    explicit MotionScene(std::string_view mission="thewarbegins"):install(mission),
        start{install.core.processState,install.priestState.movement.dude,install.priestState.movement.teleport,
            install.priestState.movement.manaBolt,install.priestState.movement.priest,geyserType,snap,retrying},
        process{install.core.processState} {
        try { install.runtime->Load(); }
        catch (...) {
            // 초기 로더 오류를 재현할 때 마지막 실제 자산 슬롯의 타입/상태/좌표를 관찰한다.
            for (std::uint32_t id=5;id<install.core.pool.Capacity();++id) {
                const auto raw=install.core.pool.Slot(Sid{static_cast<std::uint16_t>(id)});
                if (!(raw[11]&1) && raw[10]>=70 && (raw[11]&4)) std::printf("로더 미등록 자산: SID%u TYPE%u QA%u state%u\n",id,raw[10],Get(raw,12,2),raw[11]);
            }
            throw;
        }
        state.halt.battle=true;state.priest.suppressNeutralReturn=true;
        anchorState.graphReady=1;anchorState.fenceType=90;anchorState.windArcherType=140;
        anchor=std::make_unique<RawPlayerPlacementAnchor>(install.core.pool,install.core.types,install.core.hash.Entries(0),
            install.core.bookkeeping,install.additional,install.contentFinder,anchorState);
        RawCampaignMotionEffects effects;
        effects.cleanup.reserveShared=[] { return true; };
        effects.cleanup.ritual=InstallForbidden<void,Sid,Sid,std::uint32_t>();effects.cleanup.refund=InstallForbidden<void,std::uint32_t,std::uint32_t>();
        effects.halt.carrierPickup=InstallForbidden<std::uint32_t,Sid>();effects.halt.nuggetTransfer=InstallForbidden<std::uint32_t,Sid>();
        effects.carrier.genusAt=[this](float x,float y,std::uint32_t genus) {
            RawSquidFinder finder(install.core.pool,install.core.hash,install.core.types);
            // 실제 일반 genus getter의 첫 통과/skipSurface 순서를 검사 연결에서 그대로 실행한다.
            for (auto sid=finder.Begin({static_cast<int>(x),static_cast<int>(y),static_cast<int>(x),static_cast<int>(y)},(genus&6)==0 ? 1U : 0U);sid.value;sid=finder.Next())
                if (install.core.types[install.core.pool.Slot(sid)[10]].flags2&genus) return sid;
            return Sid{};
        };
        effects.carrier.cdPriestPickupPriest=InstallForbidden<bool>();effects.carrier.pickup=InstallForbidden<void,Sid>();
        effects.carrier.escape=InstallForbidden<std::uint32_t,Sid,float,std::uint32_t>();
        effects.carrier.chooseReturn=effects.carrier.chooseGeyser=InstallForbidden<Sid,Sid>();
        effects.carrier.goReturn=effects.carrier.goGeyser=InstallForbidden<void,Sid,Sid>();
        effects.carrier.minedPacket=InstallForbidden<void,Sid>();effects.carrier.cdAddSp=InstallForbidden<void,std::uint32_t,std::int32_t>();
        effects.carrier.cdOwnerPacket=InstallForbidden<void,std::uint32_t>();effects.carrier.playAt=InstallForbidden<void,float,float,std::string_view,std::uint32_t>();
        effects.carrier.robust=InstallForbidden<bool,Sid>();effects.priest.neutralReturn=InstallForbidden<void,Sid>();
        effects.path.metadata=[this](Sid sid) {
            const auto type=install.core.pool.Slot(sid)[10];const auto& definition=install.assets.game.Types()[type-70].definition;
            return RawPathStartType{static_cast<float>(definition.Number("speed").value()),
                static_cast<float>(definition.Number("turningSpeed").value()),definition.specialFrames.defaultFrame};
        };
        // 보호 사제TYPE에는 stepSound가 없으므로 실제 기본 토큰0의 요청만 관찰한다. moveSound와 혼합하지 않는다.
        effects.path.soundId=[this](Sid sid) { CHECK(!install.assets.game.Types()[install.core.pool.Slot(sid)[10]-70].definition.String("stepSound"));return 0U; };
        effects.path.stepSound=[this](Sid,float,float,std::uint32_t token) { CHECK(token==0);++steps; };
        effects.path.startBlocked=InstallForbidden<void,RawPathPoint,std::uint32_t,std::uint32_t>();
        effects.path.arrivalBlocked=InstallForbidden<void,Sid,RawPathPoint,std::uint32_t,std::uint32_t>();
        effects.path.retarget=InstallForbidden<void,RawPathRuntime&,Sid>();
        motion=std::make_unique<RawCampaignMotionBindings>(install.world,*install.priests,*install.islands,*install.contained,*anchor,
            install.hp,install.metadata.frames,state,start,process,std::move(effects));
        auto pathTransmit=motion->Paths().Transmission(header,packet,[this](std::int32_t length) { CHECK(length>0 && static_cast<std::size_t>(length)<=packet.size());++transmissions; });
        install.core.processes.SetFormTransmit(install.assetBindings->Transmission(std::move(pathTransmit)));
    }
    // Path callback이 참조하는 모든 모듈을 지우기 전에 같은 Kernel의 실제 Process를 해제한다.
    ~MotionScene() { install.world.ClearProcesses(); }
    // 현재 목록에서 지정 소유자의 실제 저장 사제를 찾는다. 별도 Squid 배열을 만들지 않는다.
    Sid Priest(std::uint32_t owner) const {
        // 실제 목록count/현재 Pool ownerBYTE를 읽는다.
        for (const auto id:install.priestState.priests.priests.Items()) {
            const auto sid=Sid{static_cast<std::uint16_t>(id)};if (install.core.pool.Slot(sid)[34]==owner) return sid;
        }
        throw std::logic_error("보호 캠페인의 실제 사제 소유자 누락");
    }
    // 실제 BFS가 사용할 같은 Graph의 비점유 도착 칸을 찾는다. 경로/결과를 직접 합성하지 않는다.
    RawPathPoint Destination(Sid priest) const {
        const auto raw=install.core.pool.Slot(priest);const auto x=static_cast<int>(std::bit_cast<float>(Get(raw,14))),y=static_cast<int>(std::bit_cast<float>(Get(raw,18)));
        const auto graph=anchor->GraphFor(priest.value);
        // 현재 받침·spot/일반 finder를 읽어 원래 섬 안의 가까운 자유 칸만 명령 입력으로 선택한다.
        for (int radius=4;radius<10;++radius) for (int dy=-radius;dy<=radius;++dy) for (int dx=-radius;dx<=radius;++dx) {
            const int px=x+dx,py=y+dy;if (px<2 || py<2 || px>=254 || py>=254 || (dx==0 && dy==0)) continue;
            if ((install.core.spots[py*256+px]&7)!=2 || anchor->GraphAt(static_cast<float>(px),static_cast<float>(py))!=graph) continue;
            RawSquidFinder finder(install.core.pool,install.core.hash,install.core.types);if (finder.Begin({px,py-2,px,py+2},1).value) continue;
            return {static_cast<float>(px)-0.5F,static_cast<float>(py)-0.5F};
        }
        throw std::logic_error("실제 캠페인의 가까운 자유 사제 목적지 누락");
    }
};
}
// 현재 실제 사제의 같은 명령/경로/도착/cleanup과 Form 반납을 검사한다.
void CheckCampaignMovement(MotionScene& scene,std::uint32_t owner) {
    const auto priest=scene.Priest(owner);CHECK(!scene.install.priests->PriestState().Immobile(priest));
    const auto destination=scene.Destination(priest);auto* path=scene.motion->Paths().StartPriest({priest,destination,1});
    CHECK(path!=nullptr && scene.motion->Paths().CurrentPath(priest)==path && scene.transmissions>0);
    const auto form=path->Form();const auto count=scene.install.core.kernel.Size();const auto begin=scene.install.core.pool.Slot(priest);
    const auto oldX=Get(begin,14),oldY=Get(begin,18);std::uint32_t frames=0;
    // 실제 Path Frame만 원본 같은 Host clock으로 진행한다. 다른34개 전투/예약 과정은 이번 검사의 범위 밖이다.
    while (auto* current=scene.motion->Paths().CurrentPath(priest)) {
        scene.install.core.processState.now+=1.0/60.0;current->RunFrame();if (++frames>1200) throw std::logic_error("실제 사제 Path 완료 시간 초과");
    }
    const auto raw=scene.install.core.pool.Slot(priest);CHECK(Get(raw,14)!=oldX || Get(raw,18)!=oldY);
    CHECK((scene.install.core.pool.Slot(form)[11]&1)!=0 && scene.install.core.kernel.Size()==count-1);
    CHECK(scene.install.core.bookkeeping.depth==0 && !scene.motion->Paths().CurrentRuntime(priest));
    CHECK(!scene.install.assets.game.Find("priest").definition.String("stepSound"));
    std::printf("실제 사제 이동: %u 프레임, sound %zu, 송신 %zu\n",frames,scene.steps,scene.transmissions);
}
// 실제1-1의 저장 적 사제도 같은 원본 저수준 명령/경로/도착/cleanup을 실행한다.
TEST_CASE(CampaignMotion_ActualWarBeginsPriestPathArrival) { MotionScene scene;CheckCampaignMovement(scene,2); }
// 실제1-2의 내 사제도 관찰 기반 Squid 배열 없이 같은 raw 경로에서 이동한다.
TEST_CASE(CampaignMotion_ActualMasterOfWhirligigsLocalPriestPath) {
    MotionScene scene("masterofwhirligigs");CheckCampaignMovement(scene,1);
}
// 캠페인1-2의 실제 초기 저장자료도 동일 상위 설치 경로로 실행한다.
TEST_CASE(CampaignMotion_ActualMasterOfWhirligigsInitialization) {
    MotionScene scene("masterofwhirligigs");const auto priest=scene.Priest(1);
    CHECK(priest.value!=0 && scene.install.core.bookkeeping.depth==0);
    CHECK(!scene.install.priests->PriestState().Immobile(priest));
}
// 원본 Level setter의 signed 진단 뒤 저장과 하위5비트 보존을 확인한다.
TEST_CASE(CampaignMotion_BaseLevelKeepsLowBitsAfterAssertion) {
    InstallScene scene;const auto sid=scene.world.Create(85);std::uint32_t assertions=0;
    RawSquidLevel level(scene.core.pool,scene.core.types,[&](SquidLevelAssertion) { ++assertions;scene.core.pool.AllocatedBytes(sid)[11]=0x14; });
    // 정상0..7은 현재 상태 하위5비트를 보존하며 이력이 아닌 현재 슬롯만 쓴다.
    for (std::uint32_t value=0;value<8;++value) { scene.core.pool.AllocatedBytes(sid)[11]=0x12;level.Set(sid,value);CHECK(level.Has(sid) && level.Get(sid)==value && (scene.core.pool.Slot(sid)[11]&31)==0x12); }
    // signed 범위 밖도 진단 후 실제 현재BYTE를 재조회해 저장한다.
    for (const auto value:{8U,0x80000000U,0xffffffffU}) { level.Set(sid,value);CHECK(level.Get(sid)==(value&7) && (scene.core.pool.Slot(sid)[11]&31)==0x14); }
    CHECK(assertions==3);
}
// 새 독립 PE 관찰96 입력을 같은 현재 슬롯의 전체BYTE와 직접 대조한다.
TEST_CASE(CampaignMotion_ReplaysActualLevelNativeBothEditions) {
    std::ifstream input(std::filesystem::path(__FILE__).parent_path()/"fixtures/squid-level-x86.tsv");CHECK(input.good());std::string line;std::size_t cases=0;
    // 각 행은 원본입력/state/group/n/진단변경과 실제Native has/level/assert/전체slot이다.
    while (std::getline(input,line)) {
        const auto row=Split(line,'\t');CHECK(row.size()==9);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        SidPool pool(edition,32768,true);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);types[82].group=static_cast<int>(Number(row[2]));
        // 실제Native의 입력 SID50까지 로컬 할당하되 슬롯/기대값은 입력BYTE로만 준비한다.
        for (std::uint32_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{static_cast<std::uint16_t>(sid)});
        const Sid sid{50};auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=82;raw[11]=static_cast<std::uint8_t>(Number(row[1]));std::uint32_t assertions=0;
        RawSquidLevel level(pool,types,[&](SquidLevelAssertion) { ++assertions;if (Number(row[4])) pool.AllocatedBytes(sid)[11]=0x14; });
        CHECK(level.Has(sid)==(Number(row[5])!=0));level.Set(sid,Number(row[3]));
        CHECK(level.Get(sid)==Number(row[6]) && assertions==Number(row[7]) && Hex(pool.Slot(sid))==row[8]);++cases;
    }
    CHECK(cases==96);
}
