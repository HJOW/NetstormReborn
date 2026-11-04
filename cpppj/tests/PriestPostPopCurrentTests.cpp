// 단위862: PATCH 사제 postPop(004950f0)의 현재 물리 슬롯·signed 목록·현재 사제 TYPE DWORD·도달 장치와 실제 가상 분배를 원문 순서로 대조한다.
// 게임·프레임 틱·포획/희생·통신은 실행하지 않으며 명시 Validate와 CD 계약이 기존대로 남는지도 함께 확인한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPostPopTail.h"
#include "o/SquidPostPop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 사제 접두/후반을 같은 풀·현재 TYPE DWORD·현재 목록으로 조립하는 작은 장면이다. 검사마다 새로 만든다.
struct PriestCurrentScene {
    bool patch; // 10.78 물리 배치 여부다.
    SidPool pool; // 관찰 SID50까지 실제 할당한 client 풀이다.
    GameRandom random; // SP 저장소 구성에만 쓰며 난수를 소비하지 않는다.
    ScrambledSpStore store; // PATCH 보상 모듈의 구성 조건이다.
    SquidPostPopState book; // 공통 후처리 장부다.
    SquidRewardState hpState; // 최대 HP getter의 현재 전역이다.
    PriestPostPopState priests; // 사제 목록과 소유자9DWORD다.
    std::vector<RiftTypeRecord> types; // TYPE158/82 HP 입력을 담은 표다.
    std::vector<RiftTypeFrames> frames; // 낙하 방향 조회용 프레임 표이며 모든 TYPE의 첫 글자는 A다.
    std::vector<std::uint8_t> spots; // 지면 비트 지도다.
    std::unique_ptr<SquidReward> hp; // 실제 최대 HP 조회 모듈이다.
    std::vector<std::string> events; // 외부 장치 호출 순서다.
    std::uint32_t priestType{kPriestType}; // 005412d0/CD0051cbbc의 현재 DWORD이며 검사 중 바꿀 수 있다.
    bool found{},allocated{true}; // 회복 검색/확보의 명시 입력이다.
    // 사제 VT/TYPE158·소유자1·원래 소유자1·좌표(20.75,21.9)·HP600의 관찰 슬롯을 만든다.
    explicit PriestCurrentScene(OriginalEdition edition=OriginalEdition::Patch1078)
        :patch(edition==OriginalEdition::Patch1078),pool(edition,kCapacity,false),store(random,[] { return 1U; }),
        types(patch ? 188 : 171),frames(types.size(),RiftTypeFrames({{'A',80,1,0}})),spots(65536) {
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id}); // 실제 할당 정책으로 관찰 SID까지 확보한다.
        Type(kPriestType,200);Type(82,600);hpState.sharedPriestType=&priestType; // 1/4 비교도 같은 현재 DWORD를 읽는다.
        hp=std::make_unique<SquidReward>(pool,types,book,hpState,patch ? &store : nullptr);
        priests.priests.entries.assign(4,0);
        for (std::size_t i=0;i<priests.ownerState.size();++i) priests.ownerState[i]=0xabc000+static_cast<std::uint32_t>(i); // 비영 소유자 칸 입력이다.
        auto raw=Slot(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{}); // 할당 뒤 남은 바이트를 지우고 원문 입력만 쓴다.
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[Owner()]=1;Put(raw,12,1,2);
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75F));Put(raw,18,std::bit_cast<std::uint32_t>(21.9F));Put(raw,26,600,patch ? 4 : 2);
    }
    // HP가 있는 사제 genus 입력이다.
    void Type(std::uint32_t number,std::int32_t value) { types[number].maxHitPoints=value;types[number].flags1=0x69012;types[number].flags2=0x210000; }
    std::size_t Owner() const { return patch ? 34 : 32; } // 현재 소유자 BYTE 위치다.
    std::span<std::uint8_t> Slot(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 할당 상태와 무관한 물리 슬롯이다.
    static std::string Bits(float value) { return std::to_string(std::bit_cast<std::uint32_t>(value)); } // payload 비트 문자열이다.
    // 검색 시점의 현재 목록 count와 확보/생성 인자를 기록한다. 최대 HP는 실제 복원 함수를 사용한다.
    PriestPostPopHooks Hooks() {
        return {[this](Sid sid,std::uint32_t event) {
            events.push_back("F:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(priests.Priests().Count()));return found;
        },[this] { events.push_back("A:40");return allocated; },[this](Sid sid,std::uint32_t event,float payload) {
            events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+Bits(payload));
        }};
    }
    // 낙하/보호막 확보/보호막 조회 경계의 호출 순서를 기록한다.
    PriestPostPopTailHooks TailHooks() {
        return {[this](Sid sid) { events.push_back("D:"+std::to_string(sid.value)); },[this](Sid sid) { events.push_back("S:"+std::to_string(sid.value)); },
            [this](Sid sid) { events.push_back("X:"+std::to_string(sid.value)); }};
    }
    // 원본 대체 경계의 기록 형식으로 합친다.
    std::string Events() const {
        std::string text;
        for (const auto& value:events) { if (!text.empty()) text+=';';text+=value; } // 발생 순서대로 구분자를 넣는다.
        return text.empty() ? "-" : text;
    }
};
// 같은 장면의 공통 장부·Carrier·사제 상태를 실제 후반 몸체와 조립한다.
struct PriestTailParts {
    SquidPostPop base; // 공통 004b0d30 장부다.
    CarrierPostPopState mode; // Carrier 부모의 현재 모드다.
    RawCarrierPostPop carrier; // 실제 00427720/0044bf80 몸체다.
    RawPriestState state; // 실제 00427030 이동 불가 조회다.
    // 장면보다 짧게 살아야 하며 Carrier 확인/지면 점유 효과는 무효과 입력이다.
    explicit PriestTailParts(PriestCurrentScene& s):base(s.pool,s.types,s.book),
        carrier(s.pool,s.types,mode,MakeCarrierPostPopHooks(s.pool,base,[](Sid) {},[](float,float,std::uint8_t) {})),state(s.pool,*s.hp,s.types,s.spots) {}
};
}
TEST_CASE(PriestPostPopCurrent_PatchPrefixReadsPhysicalSlotWithoutValidate) { // 실제 가상 진입은 TYPE/할당 사전 검사 없이 현재 물리 슬롯으로 목록·회복·소유자 칸을 쓴다.
    PriestCurrentScene s;auto raw=s.Slot(kSource);raw[10]=82;raw[11]=9;RawPriestPostPop body(s.pool,*s.hp,s.priests,s.Hooks(),s.priestType);
    CHECK(Throws([&] { body.Validate(kSource); }) && s.events.empty()); // 명시 사전 검사의 TYPE158 계약은 그대로다.
    body.Prefix(kSource,1); // 분자 TYPE158 200/6*50=1650, 객체 TYPE82 최대HP600이다.
    CHECK(s.Events()=="F:50:602:1;A:40;R:50:602:"+PriestCurrentScene::Bits(2.75F));
    CHECK(s.priests.priests.count==1 && s.priests.priests.entries[0]==50 && s.priests.ownerState[1]==0 && s.priests.ownerState[2]==0xabc002 && raw[11]==9);
    PriestCurrentScene cd(OriginalEdition::Cd1072);cd.Slot(kSource)[10]=82;RawPriestPostPop cdBody(cd.pool,*cd.hp,cd.priests,cd.Hooks(),cd.priestType); // CD는 기존 사전 계약을 유지한다.
    CHECK(Throws([&] { cdBody.Prefix(kSource,1); }) && cd.events.empty() && cd.priests.priests.count==0 && cd.priests.ownerState[1]==0xabc001);
    cd.Slot(kSource)[10]=kPriestType;cd.Slot(kSource)[11]=1;CHECK(Throws([&] { cdBody.Prefix(kSource,1); }) && cd.events.empty()); // CD의 할당 확인도 효과 전 거부다.
}
TEST_CASE(PriestPostPopCurrent_PatchSignedListUsesFirstCountAndCurrentReads) { // 00414450의 signed 관문·현재 count 재조회·처음 count 저장 위치와 음수 저장 경계를 확인한다.
    PriestCurrentScene s;RawPriestPostPop body(s.pool,*s.hp,s.priests,s.Hooks(),s.priestType);s.found=true;
    s.priests.priests.count=5;body.Prefix(kSource,1); // 용량 초과 count는 등록만 생략하고 회복 검색/소유자 쓰기를 계속한다.
    CHECK(s.Events()=="F:50:602:5" && s.priests.priests.count==5 && s.priests.ownerState[1]==0);
    s.events.clear();s.priests.ownerState[1]=7;s.priests.priests.count=0xffffffffU; // signed -1은 관문을 통과해 음수 위치에 쓰려는 호스트 물리 경계다.
    CHECK(Throws([&] { body.Prefix(kSource,1); }) && s.events.empty() && s.priests.priests.count==0xffffffffU && s.priests.ownerState[1]==7);
    s.priests.priests.count=1;s.priests.priests.entries[0]=50;s.priests.ownerState[1]=7;body.Prefix(kSource,1); // 활성 구간의 같은 SID는 추가하지 않는다.
    CHECK(s.Events()=="F:50:602:1" && s.priests.priests.count==1 && s.priests.ownerState[1]==0);
    SquidPostPopList bound;std::vector<std::uint32_t> storage{7,50,9,0},reads{2,1,1,2};std::size_t read{};std::uint32_t written{}; // 머리 count2→검색 중 현재1→증가 전 현재1→검색 기록 시 현재2다.
    bound.BindCurrent({[&] { return std::span<std::uint32_t>(storage); },[] { return 4U; },[&] { return reads.at(read++); },[&](std::uint32_t value) { written=value; }});
    s.priests.sharedPriests=&bound;s.events.clear();body.Prefix(kSource,1); // 현재 count1에서 검색을 멈춰 index1의 같은 SID를 보지 않고 처음 count2 위치에 쓴다.
    CHECK(storage==std::vector<std::uint32_t>({7,50,50,0}) && written==2 && read==4 && s.Events()=="F:50:602:2");
}
TEST_CASE(PriestPostPopCurrent_RegenNumeratorReadsCurrentPriestTypeDword) { // 0049a840(005412d0)/CD0051cbbc를 호출 시점에 읽고 최대HP 1/4 비교도 같은 현재 DWORD를 쓴다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 판본 모두 같은 현재 DWORD 계약이다.
        PriestCurrentScene s(edition);RawPriestPostPop body(s.pool,*s.hp,s.priests,s.Hooks(),s.priestType);
        s.priestType=82;body.Prefix(kSource,1); // 분자 TYPE82 600/6*50=5000, 객체 TYPE158 최대HP200이다.
        s.priestType=kPriestType;body.Prefix(kSource,1); // 같은 구성의 다음 호출은 바뀐 현재 DWORD를 읽는다: 1650/200.
        s.priestType=82;s.hpState.priestQuarterHp=true;body.Prefix(kSource,1); // TYPE82만 1/4: 150/6*50=1250, 객체 TYPE158은 현재 사제 TYPE가 아니라 200이다.
        CHECK(s.Events()=="F:50:602:1;A:40;R:50:602:"+PriestCurrentScene::Bits(25.0F)+";F:50:602:1;A:40;R:50:602:"+PriestCurrentScene::Bits(8.25F)
            +";F:50:602:1;A:40;R:50:602:"+PriestCurrentScene::Bits(6.25F));
        CHECK(s.priests.priests.count==1 && s.hp->TypeHitPoints(kPriestType)==200 && s.hp->TypeHitPoints(82)==150);
    }
}
TEST_CASE(PriestPostPopCurrent_PatchPrefixRequiresOnlyReachedHooks) { // PATCH 구성은 효과 훅을 요구하지 않고 도달한 검색/확보/생성만 그 시점에 거부한다.
    PriestCurrentScene s;RawPriestPostPop bare(s.pool,*s.hp,s.priests,{},s.priestType);auto raw=s.Slot(kSource);
    raw[40]=8;bare.Prefix(kSource,1);CHECK(s.priests.priests.count==0 && s.priests.ownerState[1]==0xabc001); // extra&9는 아무 장치/효과도 없다.
    raw[40]=0;bare.Prefix(kSource,0);CHECK(s.priests.priests.count==0 && s.priests.ownerState[1]==0); // 비최초는 소유자 칸만 쓴다.
    s.priests.ownerState[1]=7;CHECK(Throws([&] { bare.Prefix(kSource,1); }) && s.priests.priests.count==1 && s.priests.ownerState[1]==7); // 목록 등록 뒤 검색 장치에서 멈추며 소유자 칸 전이다.
    auto hooks=s.Hooks();hooks.reserveRegular={};hooks.addRegular={};s.found=true;RawPriestPostPop found(s.pool,*s.hp,s.priests,hooks,s.priestType);
    found.Prefix(kSource,1);CHECK(s.Events()=="F:50:602:1" && s.priests.ownerState[1]==0); // 기존 회복이면 확보/생성 장치에 도달하지 않는다.
    s.events.clear();s.found=false;s.priests.ownerState[1]=7;CHECK(Throws([&] { found.Prefix(kSource,1); }) && s.Events()=="F:50:602:1" && s.priests.ownerState[1]==7);
    hooks=s.Hooks();hooks.addRegular={};RawPriestPostPop reserve(s.pool,*s.hp,s.priests,hooks,s.priestType);s.events.clear();s.allocated=false;
    reserve.Prefix(kSource,1);CHECK(s.Events()=="F:50:602:1;A:40" && s.priests.ownerState[1]==0); // 확보 실패는 HP 조회/생성 장치에 도달하지 않는다.
    s.events.clear();s.allocated=true;s.priests.ownerState[1]=7;CHECK(Throws([&] { reserve.Prefix(kSource,1); }) && s.Events()=="F:50:602:1;A:40" && s.priests.ownerState[1]==7);
    PriestCurrentScene cd(OriginalEdition::Cd1072);CHECK(Throws([&] { RawPriestPostPop invalid(cd.pool,*cd.hp,cd.priests,{},cd.priestType); })); // CD 구성 사전 계약은 유지한다.
}
TEST_CASE(PriestPostPopCurrent_PatchVirtualEntryDispatchesByVtableOnly) { // PATCH 실제 가상 분배는0050f210만으로 사제 wrapper에 들어가고 명시 Validate는 TYPE158 계약을 유지한다.
    PriestCurrentScene s;s.Slot(kSource)[10]=82;s.spots[22*256+21]=6; // 현재 TYPE82·HP600≥300·이동 가능한 지면 입력이다.
    RawPriestPostPop prefix(s.pool,*s.hp,s.priests,s.Hooks(),s.priestType);PriestTailParts parts(s);
    RawPriestPostPopTail tail(s.pool,prefix,parts.carrier,parts.state,s.frames,s.spots,s.TailHooks());parts.base.SetPriestPostPop(&tail);
    CHECK(Throws([&] { parts.base.Validate(kSource,1); }) && s.events.empty()); // 명시 사전 검사는 사제 TYPE158을 계속 요구한다.
    s.book.localOwner=1;s.book.depth=1;parts.base.PostPopVirtual(kSource,1); // 접두→Carrier/공통 장부→이동 가능 보호막 조회 순서다.
    CHECK(s.Events()=="F:50:602:1;A:40;R:50:602:"+PriestCurrentScene::Bits(2.75F)+";X:50" && s.book.depth==0 && s.book.globalCounts[82]==1 && s.priests.priests.count==1);
    PriestCurrentScene cd(OriginalEdition::Cd1072);cd.Slot(kSource)[10]=82;RawPriestPostPop cdPrefix(cd.pool,*cd.hp,cd.priests,cd.Hooks(),cd.priestType);PriestTailParts cdParts(cd);
    RawPriestPostPopTail cdTail(cd.pool,cdPrefix,cdParts.carrier,cdParts.state,cd.frames,cd.spots,cd.TailHooks());cdParts.base.SetPriestPostPop(&cdTail);
    cd.book.depth=1;CHECK(Throws([&] { cdParts.base.PostPopVirtual(kSource,1); }) && cd.events.empty() && cd.priests.priests.count==0); // CD는 기존 TYPE/VT 연결 계약으로 사제 몸체에 분배하지 않는다.
}
TEST_CASE(PriestPostPopCurrent_PatchTailRequiresOnlyReachedEffects) { // 후반 구성은 효과 훅 없이 가능하고 도달한 보호막/낙하만 그 시점에 거부하며 앞선 부분 효과를 보존한다.
    PriestCurrentScene s;RawPriestPostPop prefix(s.pool,*s.hp,s.priests,s.Hooks(),s.priestType);PriestTailParts parts(s);auto raw=s.Slot(kSource);
    RawPriestPostPopTail bare(s.pool,prefix,parts.carrier,parts.state,s.frames,s.spots,{});
    raw[40]=8;s.book.depth=1;bare.PostPop(kSource,0);CHECK(s.book.depth==0 && s.events.empty() && s.priests.ownerState[1]==0xabc001); // Carrier 뒤 extra&9는 후반 장치에 도달하지 않는다.
    raw[40]=0;s.spots[22*256+21]=6;s.book.depth=1; // 이동 가능 분기의 조회 장치 누락 전에 접두/공통 장부를 끝낸다.
    CHECK(Throws([&] { bare.PostPop(kSource,1); }) && s.book.depth==0 && s.priests.priests.count==1 && s.Events()=="F:50:602:1;A:40;R:50:602:"+PriestCurrentScene::Bits(8.25F));
    raw[40]=0x20;s.events.clear();s.book.depth=1;CHECK(Throws([&] { bare.PostPop(kSource,0x800); }) && s.book.depth==0 && s.events.empty()); // 강제 이동 불가·낙하 재등록은 확보 장치에서 멈춘다.
    auto hooks=s.TailHooks();hooks.fall={};RawPriestPostPopTail noFall(s.pool,prefix,parts.carrier,parts.state,s.frames,s.spots,hooks);
    s.book.depth=1;noFall.PostPop(kSource,0);CHECK(s.Events()=="S:50" && s.book.depth==0); // 지면이 있으면 낙하 장치에 도달하지 않는다.
    s.spots[22*256+21]=0;s.events.clear();s.book.depth=1; // 지면 없음·현재 방향A(0)≠9면 낙하 장치에 도달해 보호막 확보 전에 거부한다.
    CHECK(Throws([&] { noFall.PostPop(kSource,0); }) && s.events.empty() && s.book.depth==0);
    PriestCurrentScene cd(OriginalEdition::Cd1072);RawPriestPostPop cdPrefix(cd.pool,*cd.hp,cd.priests,cd.Hooks(),cd.priestType);PriestTailParts cdParts(cd);
    CHECK(Throws([&] { RawPriestPostPopTail invalid(cd.pool,cdPrefix,cdParts.carrier,cdParts.state,cd.frames,cd.spots,{}); })); // CD 구성 사전 계약은 유지한다.
}
