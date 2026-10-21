// 업그레이드 예측 그림/완료·실제 Host 정리의 합성 검사다. 작성·컴파일만 하며 실행하지 않는다.
#include "o/RawFactoryUpgrade.h"
#include "RawSceneSupport.h"
#include <map>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 원문/Host와 명시 가상·장치 효과를 준비한다.
struct UpgradeScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true};SquidHash hash; //현재 원문과 실제 공간 해제 저장소다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188);std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); //실제 Host가 요구하는 TYPE/spot이다.
    SquidUnpop unpop{pool,hash,spots};RawSquidDestroy destroy{pool,unpop,types};Kernel kernel;SquidProcessState process; //같은 실제 예약/삭제 수명이다.
    SquidProcessHost host{pool,types,kernel,destroy,process,{},SquidDestroyHooks{[this](const SquidDestroyEvent& event) { if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();else if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy(); },[] { return Sid{}; },{}}}; //ProcessForm에는 실제 Host 가상 삭제를 쓴다.
    std::uint32_t outpost{126},dependType{1},local{2};bool checking{},challenge{},boss{true},ai{}; //현재 참조 전역/AI 입력이다.
    Sid parent{},image{};FactoryUpgradeHooks hooks;std::vector<std::string> trace;std::map<std::uint16_t,Sid> watchers; //선택 하위/현재 의존 대상/호출 순서다.
    std::vector<Sid> removed;std::vector<std::uint32_t> assertions; //실제 삭제 대상과 진단 줄이다.
    // 같은 Factory 입력과 각 가상 경계에서 관찰할 기대 조건을 구성한다.
    UpgradeScene() {
        parent=New();auto raw=pool.AllocatedBytes(parent);raw[10]=85;raw[34]=2;Put(raw,0,0x509148);Put(raw,14,std::bit_cast<std::uint32_t>(7.25F));Put(raw,18,std::bit_cast<std::uint32_t>(8.5F));
        hooks.level=[this](Sid sid) { CHECK(sid==parent);trace.push_back("level");return std::uint32_t(pool.Slot(sid)[11]>>5); };
        hooks.setLevel=[this](Sid sid,auto level) { trace.push_back("set");auto value=pool.AllocatedBytes(sid);value[11]=static_cast<std::uint8_t>((value[11]&31U)|(level<<5)); };
        hooks.maxHp=[this](Sid sid) { CHECK(sid==parent && (pool.Slot(sid)[11]>>5)==1);trace.push_back("hp");return 321; };hooks.adjustFrame=[this](Sid sid) { CHECK(Get(pool.Slot(sid),26)==321);trace.push_back("frame"); };hooks.display=[this](Sid) { trace.push_back("display"); };
        hooks.soundAt=[this](float x,float y,std::string_view name) { CHECK(x==7.25F && y==8.5F && name=="upgradeDone.wav");trace.push_back("position"); };hooks.sound=[this](std::string_view name) { CHECK(name=="upgradeComplete.wav");trace.push_back("sound"); };hooks.flare=[this](Sid sid,auto count,auto event) { CHECK(sid==parent && count==6 && event==2);trace.push_back("flare"); };
        hooks.create=[this](auto type,auto flags) { CHECK(type==85 && flags==2);trace.push_back("create");image=New();auto value=pool.AllocatedBytes(image);value[10]=85;value[11]=4;Put(value,12,0xa5a5,2);Put(value,36,77);return image; };
        hooks.owner=[this](Sid sid,auto owner) { CHECK(pool.Slot(sid)[40]==9 && Get(pool.Slot(sid),12,2)==0 && Get(pool.Slot(sid),36)==77);trace.push_back("owner");pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(owner); };
        hooks.pop=[this](Sid sid,float x,float y,auto flags) { CHECK(sid==image && x==7.25F && y==static_cast<float>(8.5+0.0010000000474974513) && flags==0 && pool.Slot(sid)[34]==2);trace.push_back("pop"); };
        hooks.construction=[this](Sid sid,float duration) { CHECK(sid==image && duration==15.0F);trace.push_back("construction"); };hooks.depend=[this](Sid watched,Sid watcher) { CHECK(watched==parent && watcher==image);trace.push_back("depend");Link(watcher); };
        hooks.watcher=[this](Sid form) { trace.push_back("watcher");return watchers.at(form.value); };hooks.destroy=[this](Sid sid,auto flags) { CHECK(flags==0);removed.push_back(sid);trace.push_back("destroy"); };
        hooks.aiPresent=[this](Sid sid) { CHECK(sid==parent);trace.push_back("ai");return ai; };hooks.notifyAi=[this](auto owner,Sid sid) { CHECK(sid==parent && owner==pool.Slot(parent)[34]);trace.push_back("notify"); };
        hooks.log=[this](auto kind,std::string_view text) { CHECK((kind==3 && text=="Upgrading!\n") || (kind==4 && text=="making the client upgrade picture"));trace.push_back("log"); };hooks.assertion=[this](auto line) { assertions.push_back(line); };
    }
    // 후속 Kernel 파괴가 하위 참조보다 먼저 끝나게 한다.
    ~UpgradeScene() { kernel.Clear(); }
    // 합성 입력은 할당 상태와 무관한 payload를 명시적으로0으로 준비한다.
    Sid New(std::uint32_t flags=2) { const auto sid=pool.Allocate(flags);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});return sid; }
    // 원본처럼 부모 head에 삽입하며 감시자 반환은 별도 명시 입력이다.
    Sid Link(Sid watched,std::uint8_t type=1) { const auto form=New();auto raw=pool.AllocatedBytes(form);raw[10]=type;Put(raw,4,Get(pool.Slot(parent),6,2),2);Put(pool.AllocatedBytes(parent),6,form.value,2);watchers[form.value]=watched;return form; }
    // 현재 전역 참조와 같은 실제 Host를 매번 빌린다.
    RawFactoryUpgrade Make() { return RawFactoryUpgrade(pool,host,{outpost,dependType,local,checking,challenge,boss},hooks); }
};
}
// 생성→두 번 레벨 조회→가상 상태→abstract/state/owner→Pop→건설→감시를 관찰한다.
TEST_CASE(FactoryUpgrade_BeginUsesActualRawOrder) {
    UpgradeScene input;input.Make().Begin(input.parent,15);CHECK(input.trace==std::vector<std::string>({"create","level","level","set","owner","pop","log","construction","depend"}));CHECK((input.pool.Slot(input.image)[11]>>5)==1 && input.Make().HasPicture(input.parent));
}
// 상한에서는 두 번째 가상 조회가 없고 하위가 바뀌면 원본 재조회 결과를 따른다.
TEST_CASE(FactoryUpgrade_NextLevelRereadsAndCaps) {
    UpgradeScene input;input.pool.AllocatedBytes(input.parent)[11]=96;CHECK(input.Make().NextLevel(input.parent)==3 && input.trace.size()==1);std::uint32_t calls{};input.hooks.level=[&](Sid) { return ++calls==1 ? 0U : 2U; };CHECK(input.Make().NextLevel(input.parent)==3 && calls==2);
}
// 최대HP 쓰기와 프레임/표시/소리·플레어·현재AI 통지 순서를 확인한다.
TEST_CASE(FactoryUpgrade_FinishOrderAndCurrentOwner) {
    UpgradeScene input;input.ai=true;input.hooks.soundAt=[&](float,float,std::string_view) { input.trace.push_back("position");input.pool.AllocatedBytes(input.parent)[34]=3;input.local=3; };CHECK(input.Make().Finish(input.parent)==-1);CHECK(Get(input.pool.Slot(input.parent),26)==321);CHECK(input.trace==std::vector<std::string>({"log","level","level","set","hp","frame","display","position","sound","flare","ai","notify"}));
}
// outpost의 완료는 무효이며 Handle의 후속AI 검사/통지는 별도로 남는다.
TEST_CASE(FactoryUpgrade_OutpostSkipsCompleteButKeepsAiNotice) {
    UpgradeScene input;input.outpost=85;input.ai=true;input.Make().Complete(input.parent);CHECK(input.trace.empty());CHECK(input.Make().Finish(input.parent)==-1 && input.trace==std::vector<std::string>({"log","ai","notify"}));
}
// 같은 실제 Host의 모든4/3 예약을 제거하되 다른 사건은 보존한다.
TEST_CASE(FactoryUpgrade_CompleteRemovesActualTimers) {
    UpgradeScene input;auto* other=input.host.AddRegular(input.parent,8,3);input.host.AddRegular(input.parent,4,20);input.host.AddSharedRegular(input.parent,4,20);input.host.AddSharedRegular(input.parent,3,15);input.host.AddRegular(input.parent,3,15);CHECK(input.Make().IsUpgrading(input.parent));input.Make().Complete(input.parent);CHECK(!input.host.FindEvent(input.parent,4) && !input.host.FindEvent(input.parent,3) && input.host.FindEvent(input.parent,8)==other && input.kernel.Size()==1 && !input.Make().IsUpgrading(input.parent));
}
// 다음링크를 먼저 보관하고 같은타입/extra8/client인 감시자만 지운다.
TEST_CASE(FactoryUpgrade_PictureFiltersAndSavedNext) {
    UpgradeScene input;const auto first=input.New(),second=input.New(),server=input.New(0),foreign=input.New();
    // 각 감시자의 명시 TYPE/extra 입력을 공급한다.
    for (auto sid:{first,second,server,foreign}) { input.pool.AllocatedBytes(sid)[10]=85;input.pool.AllocatedBytes(sid)[40]=8; }input.pool.AllocatedBytes(foreign)[10]=87;input.Link(server);input.Link(foreign);input.Link(second);const auto head=input.Link(first);input.Link(first,70);
    input.hooks.destroy=[&](Sid sid,auto flags) { CHECK(flags==0);input.removed.push_back(sid);if (sid==first) Put(input.pool.AllocatedBytes(head),4,0,2); };CHECK(input.Make().HasPicture(input.parent));input.Make().Complete(input.parent);CHECK(input.removed==std::vector<Sid>({first,second}));
}
// dead 부모/자기자신 진단과 선택된 미연결 하위의 앞선 효과를 확인한다.
TEST_CASE(FactoryUpgrade_AssertionsAndSelectedMissingEffects) {
    UpgradeScene input;input.checking=true;input.boss=false;input.pool.AllocatedBytes(input.parent)[11]=2;input.Link(input.parent);CHECK(!input.Make().HasPicture(input.parent));CHECK(input.assertions==std::vector<std::uint32_t>({0x158,0xad}));input.challenge=true;input.assertions.clear();CHECK(!input.Make().HasPicture(input.parent) && input.assertions==std::vector<std::uint32_t>({0xad}));input.hooks.construction={};CHECK(Throws([&] { input.Make().Begin(input.parent,15); }));CHECK(input.trace.back()=="log" && input.pool.Slot(input.image)[40]==9);
}
