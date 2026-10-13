// 실제 Dais Bridge 명령을 같은 PathStart/BFS/Host/cleanup과 독립 PE 전체 경로 비트에 연결한다.
#include "RawSceneSupport.h"
#include "client/RawPriestWorldBindings.h"
#include "o/RawDaisRegular.h"
#include "o/RawPathCleanup.h"
#include "o/RawPathData.h"
#include "o/RawPathSearch.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 선택하지 않은 의식/장치/도착 효과를 실행 가능한 빈 효과로 바꾸지 않는다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> BridgeForbidden() {
    return [](Arguments...)->Result { throw std::logic_error("Bridge 명령의 미선택 하위 효과"); };
}
// 현재 입력 프레임/첫표/핫스폿은 기존 원본과 같은 합성 자산이다.
void BridgeAssets(std::vector<RiftTypeRecord>& types,std::vector<RiftTypeFrames>& frames,std::vector<SquidDisplayShape>& shapes) {
    const std::array<std::pair<std::uint32_t,std::string>,4> patterns{{{158,"AEKLMNOP"+std::string(24,'P')},{164,std::string(3,'A')+std::string(4,'O')+std::string(3,'M')+std::string(3,'L')+std::string(3,'P')},{165,std::string(180,'A')},{166,std::string(6,'A')}}};
    // 출력 목표/경로를 계산하지 않고 같은 입력 코드를 실제 RiftTypeFrames에 넘긴다.
    for (const auto& [type,pattern]:patterns) {
        types[type].footX=types[type].footY=type==165 ? 3 : 1;types[type].maxHitPoints=200;types[type].flags1=16;std::vector<FrameCode> codes;
        // 원본의 signed short 핫스폿은 Temple.StagePoint가 실제로 읽는다.
        for (std::size_t index=0;index<pattern.size();++index) { codes.push_back({static_cast<std::uint8_t>(pattern[index]),'P',static_cast<std::uint8_t>(index+1),0});shapes[type].frames.push_back({1,1,static_cast<std::int16_t>((index%5)*16),static_cast<std::int16_t>((index%5)*11),1,1}); }
        frames[type]=RiftTypeFrames(std::move(codes));shapes[type].loaded=true;shapes[type].frameCount=static_cast<std::int32_t>(pattern.size());
    }
    types[158].flags2=0x210000;types[83].footX=types[83].footY=1;types[83].flags1=0x800;types[83].flags2=2;frames[83]=RiftTypeFrames({{'A','A',1,0}});
}
// 실제 꼭짓점/방향/시각 비트를 원본과 같은 구분자로 읽는다.
std::string BridgePoints(const RawPathRuntime& runtime) {
    std::string result;
    // 꼭짓점 순서는 실제 BFS와 Prepare가 정한다.
    for (std::int32_t index=0;index<runtime.Count();++index) { if (!result.empty()) result+=';';const auto point=runtime.Point(index);result+=std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+std::to_string(std::bit_cast<std::uint32_t>(point.y))+','+std::to_string(runtime.Direction(index))+','+std::to_string(std::bit_cast<std::uint64_t>(runtime.Time(index))); }
    return result.empty() ? "-" : result;
}
// 원본 allocator/이전 경로 Kill만 공통 사건으로 비교한다. Host/Frame/반납은 실제 저장소를 검사한다.
std::string BridgeEffects(const std::vector<std::string>& effects) {
    std::string result;
    // 명시 Native Host/heap 해제/Frame 경계의 사건은 별도 실제 C++ 저장소 검사로 확인한다.
    for (const auto& effect:effects) if (effect=="C" || effect=="K") { if (!result.empty()) result+=';';result+=effect; }
    return result.empty() ? "-" : result;
}
}

// 같은 풀의 새/기존 실제 Path와 명령 꼬리/마지막 점/두 번째 Prepare/깊이를 전수 대조한다.
TEST_CASE(rawDaisBridge_native_actual_path_start_prepare_depth_and_host) {
    const auto fixture=LoadFixture(NETSTORM_DAISBRIDGE_FIXTURE);std::size_t checked=0;
    // 각 입력은 자체 풀/해시/Host/프레임/전역을 사용한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==7);std::map<std::string,std::string> input;
        // 원본 입력 JSON의 정수만 읽으며 결과 경로를 재계산하지 않는다.
        for (const auto& field:Split(std::string_view(row[1]).substr(1,row[1].size()-2),',')) { const auto split=field.find(':');input.emplace(field.substr(1,split-2),field.substr(split+1)); }
        const auto number=[&](const char* name) { return Number(input.at(name)); };const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078,boss=number("boss")!=0,legacy=number("legacy")!=0;SidPool pool(edition,24000,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());BridgeAssets(types,frames,shapes);
        // Native의 모든 입력 SID를 실제 client free list에서 확보한다.
        for (std::uint16_t sid=5;sid<221;++sid) { CHECK(pool.Allocate(2)==Sid{sid});auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        const auto insert=[&](Sid sid,std::uint32_t type,float x,float y,int level) { auto raw=pool.AllocatedBytes(sid);Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));raw[patch ? 33 : 31]=static_cast<std::uint8_t>(level);auto& head=hash.Bucket(level,x,y);Put(raw,4,head,2);head=sid.value; };
        insert(Sid{5},158,9.2F,9.3F,1);insert(Sid{60},165,11.75F,11.25F,1);insert(Sid{61},164,11.75F,11.25F,1);Put(pool.AllocatedBytes(Sid{60}),0,patch ? kPatchDaisVtable : kCdDaisVtable);Put(pool.AllocatedBytes(Sid{60}),patch ? 36 : 34,3+11*number("stage"),patch ? 4 : 1);std::uint16_t next=100;
        // 입력 표면을 실제 hash0/Graph3/spot6에 넣는다. 경로는 기존 실제 BFS가 만든다.
        for (int y=9;y<20;++y) {
            for (int x=9;x<20;++x) { const Sid sid{next++};insert(sid,83,static_cast<float>(x),static_cast<float>(y),0);pool.AllocatedBytes(sid)[patch ? 30 : 28]=3;spots[y*256+x]=6; }
        }
        // blocked 입력은 표면 머리만 지우며 사제/알터/포획 자격 조건을 유지한다.
        if (number("blocked")) for (int y=0;y<256;++y) for (int x=0;x<256;++x) hash.Cell(0,x,y)=0;
        for (const auto sid:{5U,60U,61U}) pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(sid)})[patch ? 34 : 32]=1;
        Put(pool.AllocatedBytes(Sid{5}),26,200,patch ? 4 : 2);Put(pool.AllocatedBytes(Sid{60}),6,63,2);auto captured=pool.AllocatedBytes(Sid{63});Put(captured,0,kVtable);captured[10]=6;Put(captured,12,2,2);Put(captured,18,158,2);Put(captured,20,32,2);
        SquidPop pop(pool,hash,spots);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel(32);SquidProcessState clock;clock.now=123.125;clock.boss=boss;RawPathProcessState pathState{clock};SquidPostPopState book;SquidPostPop post(pool,types,book);SquidDestroyHooks deletion;
        deletion.selected=[] { return Sid{}; };deletion.emit=[](const SquidDestroyEvent& event) { CHECK(event.effect==SquidDestroyEffect::Transmit); };SquidProcessHost host(pool,types,kernel,destroy,clock,{},deletion);RawPathSearchState searchState;RawPathSearch search(pool,hash,spots,types,frames,searchState);std::vector<std::string> effects;std::uint32_t transmissions=0,frameUpdates=0;RawTempleRitual* ritualPointer=nullptr;
        RawPathDataHeader packetHeader;std::vector<std::uint8_t> packet(2048,0xa5);host.SetFormTransmit(MakePathFormTransmission(pool,types,packetHeader,packet,[&](std::int32_t length) { CHECK(length>0 && Get(packet,6,2)==5);++transmissions; }));
        SquidFrameHooks frameHooks;frameHooks.frameSize=[](Sid,std::int32_t) { return std::array<float,2>{1,1}; };frameHooks.update=[&](Sid sid,std::uint32_t flags) { CHECK(sid==Sid{5} && flags==0);++frameUpdates; };frameHooks.unpop=[&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,types[pool.Slot(sid)[10]],flags); };frameHooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(pop.Pop(sid,types[pool.Slot(sid)[10]],1,1,x,y,flags)==RawPopResult::Registered); };SquidFrame frame(pool,types,std::move(frameHooks));RawContainedLookup lookup(pool,BridgeForbidden<void,Sid,std::uint32_t>());ContainedFinderState containedState;CarrierPreDestroyHooks carrierHooks;
        carrierHooks.validateDamageable=BridgeForbidden<void,Sid,std::uint32_t>();carrierHooks.damageablePre=BridgeForbidden<void,Sid,std::uint32_t>();carrierHooks.dependentFound=BridgeForbidden<void>();RawCarrierPreDestroy carrier(pool,containedState,std::move(carrierHooks));PathCleanupState cleanupState;cleanupState.boss=boss;PathCleanupHooks cleanupHooks;
        cleanupHooks.reserveShared=BridgeForbidden<bool>();cleanupHooks.addShared=BridgeForbidden<void,Sid,std::uint32_t,float>();cleanupHooks.contained=[&](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { return carrier.FindContained(sid,mask,kind,index); };cleanupHooks.graphFor=BridgeForbidden<std::uint32_t,Sid>();cleanupHooks.ritual=[&](Sid dais,Sid priest,std::uint32_t mode) { CHECK(ritualPointer);ritualPointer->Ritual(dais,priest,mode); };cleanupHooks.refund=BridgeForbidden<void,std::uint32_t,std::uint32_t>();cleanupHooks.destroy=BridgeForbidden<void,Sid,std::uint32_t>();RawPathCleanup cleanup(pool,hash,types,cleanupState,std::move(cleanupHooks));
        const auto processHooks=[&](bool old) { RawPathProcessHooks hooks;hooks.unpop=[&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,types[pool.Slot(sid)[10]],flags); };hooks.pop=[&](Sid sid,RawPathPoint point,std::uint32_t flags) { CHECK(pop.Pop(sid,types[pool.Slot(sid)[10]],1,1,point.x,point.y,flags)==RawPopResult::Registered); };hooks.repop=[&](Sid sid,std::uint32_t flags) { const auto raw=pool.Slot(sid);CHECK(pop.Pop(sid,types[raw[10]],1,1,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),flags)==RawPopResult::Registered); };hooks.halt=BridgeForbidden<std::uint32_t,Sid>();hooks.arrival=BridgeForbidden<std::uint32_t,Sid,std::uint32_t,std::uint32_t>();hooks.stepSound=BridgeForbidden<void,Sid,float,float,std::uint32_t>();hooks.soundId=BridgeForbidden<std::uint32_t,Sid>();hooks.clearBlocked=BridgeForbidden<void,Sid,RawPathPoint,std::uint32_t,std::uint32_t>();hooks.cleanup=[&,old](RawPathRuntime& runtime,Sid parent,std::uint32_t) { CHECK(parent==Sid{5});if (old) effects.push_back("K");cleanup.AfterSpatial(runtime,parent); };return MakePathSearchHooks(pool,search,std::move(hooks)); };
        Sid oldForm{}; // 객체 메모리 재사용과 관계없이 실제 이전 Form SID를 구분한다.
        if (legacy) { auto old=std::make_unique<RawPathProcess>(pool,hash,types,frames,shapes,spots,pathState,processHooks(true),true);old->Runtime().SetMoveAwayDepth(static_cast<std::int32_t>(number("depth")));auto* attached=RawPathProcess::Attach(host,std::move(old),Sid{5},0x10);CHECK(attached);if (attached) oldForm=attached->Form(); }
        std::uint32_t dude=70,teleport=134,mana=105,priestType=158,geyser=122;bool snap=false,retry=false;netstorm::client::RawMovementProcessTypes mapper;RawPathStartHooks startHooks;
        const auto processType=[&](std::uint32_t type) { return netstorm::client::SelectMovementProcessType(edition,types,mapper,type); };const auto metadata=[](Sid) { return RawPathStartType{0.7F,90,7}; };startHooks.processType=processType;startHooks.metadata=metadata;startHooks.clearBlocked=BridgeForbidden<void,RawPathPoint,std::uint32_t,std::uint32_t>();startHooks.create=[&](bool priest) { CHECK(priest);effects.push_back("C");return std::make_unique<RawPathProcess>(pool,hash,types,frames,shapes,spots,pathState,processHooks(false),priest); };RawPathStart path(pool,hash,types,host,lookup,frame,{clock,dude,teleport,mana,priestType,geyser,snap,retry},std::move(startHooks));
        RawSquidNeighbors footprint(pool,hash,spots,types,frames);SquidRewardState rewardState;GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidReward reward(pool,types,book,rewardState,patch ? &store : nullptr);PriestRitualActionHooks actionHooks;
        actionHooks.setFrame=BridgeForbidden<void,Sid,std::int32_t,std::uint32_t>();actionHooks.reserveShared=BridgeForbidden<bool>();actionHooks.addShared=BridgeForbidden<void,Sid,std::uint32_t,float>();RawPriestRitualActions actions(pool,frames,std::move(actionHooks));TempleRitualState ritualState;ritualState.boss=boss;TempleRitualHooks ritualHooks;
        ritualHooks.create=BridgeForbidden<Sid,std::uint32_t,std::uint32_t>();ritualHooks.setHitPoints=BridgeForbidden<void,Sid,std::int32_t>();ritualHooks.setOwner=BridgeForbidden<void,Sid,std::uint32_t>();ritualHooks.pop=BridgeForbidden<void,Sid,float,float,std::uint32_t>();ritualHooks.playAt=BridgeForbidden<void,float,float,std::string_view,std::uint32_t>();ritualHooks.reserveShared=BridgeForbidden<bool>();ritualHooks.addShared=BridgeForbidden<void,Sid,std::uint32_t,float>();RawTempleRitual ritual(pool,hash,types,frames,shapes,spots,footprint,reward,actions,ritualState,std::move(ritualHooks));ritualPointer=&ritual;RunePostPopState runeState;RunePostPopHooks runeHooks;
        runeHooks.reserveRegular=BridgeForbidden<bool>();runeHooks.addRegular=BridgeForbidden<void,Sid,std::uint32_t,float>();runeHooks.startConstruction=BridgeForbidden<void,Sid,float>();RawRunePostPop rune(pool,hash,types,runeState,post,std::move(runeHooks));std::uint32_t localOwner=1;DaisRegularState state{ritualState,runeState,clock,rewardState,localOwner};DaisRegularHooks hooks;
        hooks.setFrame=BridgeForbidden<void,Sid,std::int32_t,std::uint32_t>();hooks.destroy=BridgeForbidden<void,Sid,std::uint32_t>();hooks.reserveShared=BridgeForbidden<bool>();hooks.addShared=BridgeForbidden<void,Sid,std::uint32_t,float>();hooks.playAt=BridgeForbidden<void,float,float,std::string_view,std::uint32_t>();hooks.reserveSound=BridgeForbidden<bool>();hooks.loadSound=BridgeForbidden<std::uint32_t,std::string_view,std::uint32_t,std::uint32_t>();hooks.attachSound=BridgeForbidden<std::uintptr_t,Sid,std::uint32_t,std::uint32_t,std::uint32_t>();hooks.wallSeconds=BridgeForbidden<double>();hooks.setSoundStart=BridgeForbidden<void,std::uintptr_t,double>();hooks.stopVoice=BridgeForbidden<void,std::uint32_t,std::uint32_t,std::uint32_t>();hooks.processType=processType;hooks.pathMetadata=metadata;hooks.damageable=BridgeForbidden<float,Sid,std::uint32_t,std::uint32_t,float>();hooks.addLocalSp=BridgeForbidden<void,float>();hooks.normalizeForeignSp=BridgeForbidden<void,std::uint32_t>();hooks.grantForeignType=BridgeForbidden<void,std::uint32_t,std::uint32_t>();hooks.localSpecial=BridgeForbidden<void,std::uint32_t>();hooks.localType=BridgeForbidden<void,std::uint32_t>();hooks.refreshOwner=BridgeForbidden<void,std::uint32_t>();hooks.refreshLocal=BridgeForbidden<void>();hooks.priestExists=BridgeForbidden<bool,std::uint32_t>();hooks.multiplayerStatus=BridgeForbidden<std::uint32_t>();hooks.chat=BridgeForbidden<void,const DaisChat&>();RawDaisRegular dais(pool,hash,types,frames,ritual,rune,carrier,host,lookup,path,state,std::move(hooks));
        dais.BridgeCommand(Sid{60},number("explicit") ? Sid{5} : Sid{},number("current"));const auto form=lookup.Find(Sid{5},54);auto* current=form.value ? dynamic_cast<RawPathProcess*>(host.FromForm(form)) : nullptr;const bool created=Number(row[3])!=0;
        CHECK(BridgeEffects(effects)==BridgeEffects(Split(row[2],';')));
        if (created) { CHECK(current && form!=oldForm);if (current) { CHECK(static_cast<std::uint32_t>(current->Runtime().Count())==Number(row[3]) && BridgePoints(current->Runtime())==row[5]);const auto layout=RawPathRuntime::Layout(edition);const auto raw=current->Runtime().Bytes();std::string command;
            // 정확한 명령 DWORD/깊이 필드만 읽고 Native와 대조한다.
            for (const auto offset:std::array<std::size_t,6>{{layout.targetA,layout.targetB,layout.helper,patch ? 0x186e4U : 0x2f1cU,patch ? 0x186e8U : 0x2f20U,patch ? 0x186d8U : 0x2f18U}}) { if (!command.empty()) command+=',';command+=std::to_string(Get(raw,offset)); }CHECK(command==row[4]);CHECK(form.value>=pool.Layout().serverFirst && Get(pool.Slot(form),16,2)==5 && current->Parent()==Sid{5}); }CHECK(transmissions==1 && frameUpdates==2 && kernel.Size()==1); }
        else { CHECK(transmissions==0 && frameUpdates==0);if (BridgeEffects(effects).find('K')!=std::string::npos) CHECK(!current && kernel.Size()==0);else CHECK((current!=nullptr)==legacy && kernel.Size()==(legacy ? 1U : 0U)); }
        // Kernel 종료는 destructor 경계이며 아직 경로 걷기/가상 도착의 완료를 주장하지 않는다.
        kernel.Clear();++checked;
    }
    CHECK(checked==640);
}
