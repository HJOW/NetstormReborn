// 실제 양판본 부모/TYPE HP 관측을 같은 raw 선택기에 재생한다. 게임·공격·MP는 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawBurningSources.h"
#include "o/RawGameAuxState.h"
#include <memory>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 입자/픽셀 접근 없이 높이2의 원문 이른 복귀를 구성한다.
struct CountScene { // 같은 풀/TYPE/장기 전역/현재 진단을 실제 연소점 선택기에 공급한다.
    SidPool pool; // 선택 피해50/대체 피해51의 실제 raw 저장소다.
    GameRandom random; // 높이2 계산에서는 소비하지 않아야 한다.
    std::vector<RiftTypeRecord> types; // 처음 선택한 TYPE this와 후단 현재 TYPE 조회가 공유하는 표다.
    RawGameAuxState owner; // 실제 사제/dais/약화의 장기 owner와 같은 자료형이다.
    BurningSourcesState mode; // 원문 checking/진단과 같은 owner 주소를 빌린다.
    BurningVisualState visualState; // 사용하지 않는 실제 Append 상태다.
    std::unique_ptr<RawRenderRegistry> registry; // 표시 장치는 선택되면 명시 실패한다.
    std::unique_ptr<RawBurningVisual> visual; // 같은 풀의 실제 Append 본체다.
    std::unique_ptr<RawBurningSources> sources; // 제품 코드의 실제 선택 부모다.
    BurningVisualFields fields; // 각 독립 입력의 피해 번호/이미지/목표 행이다.
    std::uint32_t checking{},profile{},robustCalls{}; // 현재 DWORD/명시 복귀 프로필/앞단 장치 횟수다.
    std::string diagnostics; // 원문 식/파일/줄의 순서를 기록한다.
    CountScene(OriginalEdition edition):pool(edition,24000),types(edition==OriginalEdition::Patch1078 ? 188 : 171) { // 현재 타입 표의 주소를 이후 재할당하지 않는다.
        for (std::uint16_t id=5;id<=51;++id) CHECK(pool.Allocate(2)==Sid{id}); // 원문 입력의 두 실제 피해 슬롯까지 확보한다.
        RenderRegistryHooks registration;registration.reserve=[](auto)->bool { throw std::logic_error("높이2의 표시 확보 금지"); };registration.releaseArray=[] { throw std::logic_error("높이2의 표시 반납 금지"); };registration.attach=[](auto&,auto,auto,auto) { throw std::logic_error("높이2의 표시 부착 금지"); };registry=std::make_unique<RawRenderRegistry>(pool,std::move(registration));
        BurningVisualHooks hooks;hooks.reserve=[](auto)->bool { throw std::logic_error("높이2의 입자 확보 금지"); };visual=std::make_unique<RawBurningVisual>(pool,*registry,visualState,std::move(hooks));
        mode.victim.checking.Bind(checking);mode.victim.robust=[this](auto,bool) { ++robustCalls;return true; };mode.sharedWeakenedMana=&owner.PriestQuarterHp();mode.sharedManaType=&owner.PriestType();mode.sharedDaisType=&owner.DaisType();
        mode.victim.assertion=[this](auto expression,auto file,auto line) { // 같은 원문 진단이 복귀하는 동안 명시 입력만 변경한다.
            if (!diagnostics.empty()) diagnostics+=';';diagnostics+=std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line);CHECK(line==0x1e3);
            if (profile) {
                auto raw=pool.AllocatedBytes(Sid{50});Put(raw,26,profile==2 ? 5U : 0U);types[74].maxHitPoints=200;owner.PriestQuarterHp()=profile!=3;owner.PriestType()=profile==3 ? 75 : 74;
                if (profile==2 || profile==3) { raw[10]=75;fields.victim=Sid{51};types[75].flags2=profile==2 ? 0x400U : 0U; }
                if (profile==3) owner.DaisType()=75;
            }
        };
        sources=std::make_unique<RawBurningSources>(pool,types,*visual,random,mode);
    }
    void Prepare(const std::vector<std::string>& input) { // 원문 관측기와 같은 초기 물리 입력을 설치한다.
        CHECK(input.size()==8);std::fill(types.begin(),types.end(),RiftTypeRecord{});types[74].maxHitPoints=std::stoi(input[1]);checking=Number(input[3]);types[74].flags1=checking ? 0 : 16;types[74].flags2=0x400000U|(Number(input[4]) ? 0x400U : 0U);types[75].maxHitPoints=999;
        owner.PriestQuarterHp()=Number(input[2])!=0;owner.PriestType()=Number(input[6]);owner.DaisType()=165;profile=Number(input[7]);diagnostics.clear();robustCalls=0;
        fields={};fields.victim=Sid{50};fields.image.width=std::stoi(input[5]);fields.image.height=2;auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=74;raw[pool.Edition()==OriginalEdition::Patch1078 ? 40 : 35]=1;Put(raw,26,std::bit_cast<std::uint32_t>(std::stoi(input[0])),pool.Edition()==OriginalEdition::Patch1078 ? 4 : 2);
    }
    std::string Post() { // 반환 슬롯/피해 번호/현재 전역/선택 TYPE HP의 독립 후상태를 조립한다.
        const auto raw=pool.Slot(Sid{50});const auto hp=pool.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Get(raw,26)) : std::int32_t(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Get(raw,26,2))));
        return std::to_string(fields.selected)+','+std::to_string(hp)+','+std::to_string(raw[10])+','+std::to_string(fields.victim.value)+','+std::to_string(checking)+','+std::to_string(owner.PriestQuarterHp() ? 1 : 0)+','+std::to_string(owner.PriestType())+','+std::to_string(owner.DaisType())+','+std::to_string(types[74].maxHitPoints);
    }
};
}

TEST_CASE(BurningSourceCountCurrent_Native5652ActualGetterArithmeticAndResumedState) { // 높이2 부모의 정상/실제IDIV예외와 진단 뒤 선택/전역을 전부 원문과 대조한다.
    std::unique_ptr<CountScene> scene;std::string editionName;std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_BURNING_SOURCE_COUNT_FIXTURE).rows) { // 기대값은 기존 C++ 수식으로 만들지 않는다.
        CHECK(row.size()==7);if (editionName!=row[0]) { editionName=row[0];scene=std::make_unique<CountScene>(editionName=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072); }
        scene->Prepare(Split(row[1],','));const auto seed=scene->random.State(),free=scene->pool.FreeCount();std::string outcome="ok";
        try { scene->sources->Select(scene->fields); }catch (const std::domain_error&) { outcome="zero"; }catch (const std::overflow_error&) { outcome="overflow"; }
        const auto post=scene->Post();const auto diag=scene->diagnostics.empty() ? "-" : scene->diagnostics;const bool same=outcome==row[2] && post==row[3] && diag==row[4];
        if (!same) { std::printf("%s %s\nactual %s %s %s\nnative %s %s %s\n",row[0].c_str(),row[1].c_str(),outcome.c_str(),post.c_str(),diag.c_str(),row[2].c_str(),row[3].c_str(),row[4].c_str());CHECK(same);return; }
        if (scene->pool.Edition()==OriginalEdition::Patch1078) CHECK(scene->robustCalls==Number(row[6])); // CD의 기존 유효SID 앞단 계약은 신규 robust 복원으로 세지 않는다.
        CHECK(scene->random.State()==seed && scene->pool.FreeCount()==free && scene->fields.total==0 && scene->fields.particles.empty());++rows;
    }
    CHECK(rows==5652);
}

TEST_CASE(BurningSourceCountCurrent_UsesLongOwnerAndCurrentDaisException) { // 지역158/165 사본과 같은 장기 번호/약화의 실제 효과를 구별한다.
    CountScene scene(OriginalEdition::Patch1078);scene.Prepare({"50","200","1","0","0","30","74","0"});scene.sources->Select(scene.fields);CHECK(scene.fields.selected==1);
    scene.owner.PriestType()=75;scene.fields.selected=0;scene.sources->Select(scene.fields);CHECK(scene.fields.selected==4 && scene.mode.manaType==158);
    scene.types[74].flags1=0;scene.checking=0x80000000;scene.owner.PriestQuarterHp()=false;scene.owner.DaisType()=74;scene.mode.victim.assertion={};scene.fields.selected=0;scene.sources->Select(scene.fields);CHECK(scene.fields.selected==4 && scene.diagnostics.empty());
    scene.owner.DaisType()=75;scene.fields.selected=0;CHECK(Throws([&] { scene.sources->Select(scene.fields); }));CHECK(scene.fields.selected==0);
}

TEST_CASE(BurningSourceCountCurrent_ReplacedDiagnosticKeepsCapturedLifetime) { // HP 진단이 자기 연결을 반납해도 현재 getter 복귀와 이전 HP 저장을 보존한다.
    CountScene scene(OriginalEdition::Patch1078);scene.Prepare({"50","200","0","2147483648","0","30","74","0"});auto pin=std::make_shared<int>(17);std::weak_ptr<int> weak=pin;
    scene.mode.victim.assertion=[&,keep=pin](auto,auto,auto) { scene.mode.victim.assertion={};CHECK(*keep==17);Put(scene.pool.AllocatedBytes(Sid{50}),26,0);scene.checking=0; };pin.reset();
    scene.sources->Select(scene.fields);CHECK(scene.fields.selected==4 && Get(scene.pool.Slot(Sid{50}),26)==0 && weak.expired());
}
