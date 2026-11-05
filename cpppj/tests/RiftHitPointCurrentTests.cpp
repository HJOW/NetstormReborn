// TYPE HP 원문/진단 복귀와 실제 공장·장기 번호만 검사한다. 게임/의식/공중3종/MP는 실행하지 않는다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "RawSceneSupport.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawPriestWorldBindings.h"
#include "o/RawRiftHitPoints.h"
#include "o/SquidFactory.h"
#include <optional>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 명시 원문 입력과 실제 물리 Record를 연결한다.
template<class F> bool HpFails(F&& operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } // 선택 장치 실패/호스트 주소 경계를 관찰한다.
}

TEST_CASE(RiftHitPointCurrent_Native1160BothEditionsAndResumedPhysicalType) { // 진단 뒤 같은 this/바뀐 base·현재 전역/음수HP를 모두 원문과 대조한다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_RIFT_HIT_POINT_FIXTURE).rows) { // 원본 입력과 독립 반환/후상태를 한 행씩 읽는다.
        CHECK(row.size()==6);const auto input=Split(row[1],',');CHECK(input.size()==9);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        std::vector<RiftTypeRecord> physical(190);const auto number=Number(input[0]);auto& selected=physical[1+number];
        selected.maxHitPoints=std::stoi(input[1]);selected.flags1=Number(input[2]);std::uint32_t checking=Number(input[3]),quarter=Number(input[4]),priest=Number(input[5]),dais=Number(input[6]),calls{};
        auto shift=std::stoi(input[7]);const auto profile=Number(input[8]);std::string diagnostics;
        const auto origin=std::span<const RiftTypeRecord>(physical).subspan(1,188);
        RiftHitPointHooks hooks;
        hooks.number=[&] { ++calls;return CurrentRiftTypeNumber(selected,origin,std::span<const RiftTypeRecord>(physical).subspan(static_cast<std::size_t>(1+shift),188)); };
        hooks.assertion=[&](auto expression,auto file,auto line) {
            if (!diagnostics.empty()) diagnostics+=';';diagnostics+=std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line);
            if (profile) { checking=0;quarter=0x80000000;selected.maxHitPoints=std::array<std::int32_t,3>{-13,-37,41}[profile-1];shift=std::array<int,3>{0,1,-1}[profile-1];priest=static_cast<std::uint32_t>(74-shift); }
        };
        const auto actual=ReadRiftHitPoints(edition,selected,{checking,quarter,priest,dais},hooks);
        const auto post=std::to_string(selected.maxHitPoints)+','+std::to_string(checking)+','+std::to_string(quarter)+','+std::to_string(priest)+','+std::to_string(dais)+','+std::to_string(shift);
        CHECK(actual==std::stoi(row[2]) && (diagnostics.empty() ? "-" : diagnostics)==row[3] && calls==Number(row[4]) && post==row[5]);++count;
    }
    CHECK(count==1160);
}

TEST_CASE(RiftHitPointCurrent_FactoryKeepsSelectedTypeAcrossRebasedDiagnosis) { // 실제PostCreate는 진단이raw TYPE/base를 바꿔도 저장된 this의HP를 쓴다.
    SidPool pool(OriginalEdition::Patch1078,32768);std::vector<RiftTypeRecord> physical(189);auto source=std::span<const RiftTypeRecord>(physical).first(188);
    physical[74].flags2=0x400000;physical[74].maxHitPoints=29;physical[74].zOrder=17;physical[75].maxHitPoints=999;
    SquidFactory factory(pool,source);bool quarter=false;std::uint32_t priest=158,dais=165,checking=0x80000000,calls{};
    factory.BindWeakenedMana(quarter);factory.BindHitPointTypes(&priest,&dais);const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=74;raw[40]=1;raw[34]=9;
    factory.BindDiagnostics({source,[&](auto expression,auto file,auto line) {
        CHECK(expression=="(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType" && file=="\\Ns\\O\\Rifttype.cpp" && line==0x1e3);
        CHECK(raw[35]==17 && raw[34]==0);++calls;quarter=true;priest=73;physical[74].maxHitPoints=-37;raw[10]=75;
        factory.BindDiagnostics({std::span<const RiftTypeRecord>(physical).subspan(1,188),{},{},checking});
    },{},checking});
    factory.PostCreate(sid);
    CHECK(calls==1 && std::bit_cast<std::int32_t>(Get(raw,26))==-9 && raw[10]==75 && raw[35]==17 && raw[34]==0);
}

TEST_CASE(RiftHitPointCurrent_SelectedMissingDiagnosticKeepsEarlierFactoryWrites) { // HP 앞의owner/zOrder와 아직 쓰지 않은HP를 구별한다.
    SidPool pool(OriginalEdition::Patch1078,32768);std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);types[74].flags2=0x400000;types[74].zOrder=0x123;types[74].maxHitPoints=91;
    SquidFactory factory(pool,types);std::uint32_t checking=1;factory.BindDiagnostics({types,{},{},checking});const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=74;raw[40]=1;raw[34]=9;Put(raw,26,0x12345678);
    CHECK(HpFails([&] { factory.PostCreate(sid); }) && raw[35]==0x23 && raw[34]==0 && Get(raw,26)==0x12345678);
    RiftTypeRecord type;type.maxHitPoints=-7;std::uint32_t priest=74,dais=74;CHECK(ReadRiftHitPoints(OriginalEdition::Patch1078,type,{false,false,priest,dais},{})==-7);
    CHECK(ReadRiftHitPoints(OriginalEdition::Cd1072,type,{true,false,priest,dais},{})==-7); // CD는 checking 진단과 미선택번호장치를 요구하지 않는다.
}

TEST_CASE(RiftHitPointCurrent_ActualWorldsUseSameLongLivedPriestAndDais) { // World 재대여와 실제 이동/회복 참조가 동일한 현재 번호를 보존한다.
    std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);
    auto globals=std::make_shared<RawGameAuxState>();globals->PriestType()=74;globals->DaisType()=74;globals->PriestQuarterHp()=true;
    auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr,std::shared_ptr<RawAuthorityState>{},globals);
    std::uint32_t checking=0x80000000,diagnostics{};std::optional<std::string> detail;storage->BindChecking(checking);
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType" && file=="\\Ns\\O\\Rifttype.cpp" && line==0x1e3);++diagnostics;storage->types[74].maxHitPoints=129; });
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    for (std::size_t generation=0;generation<2;++generation) { // 새 공장이 이전 Session의 지역 번호 주소나158 사본을 보관하지 않는다.
        globals->PriestType()=74;globals->DaisType()=74;storage->types[74].flags1=TypeFlag1::kHasHitPoints;storage->types[74].flags2=0;storage->types[74].maxHitPoints=100;
        RawPriestWorldState priest(&globals->PriestType());CHECK(&priest.movement.priest==&globals->PriestType());
        RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,frames,shapes,links,83,nullptr,storage);globals->PriestQuarterHp()=true; // 일반 World 설치의 기존 약화0 뒤 명시 현재 옵션 입력을 공급한다.
        auto core=world.Core();const auto sid=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(sid);CHECK(Get(raw,26)==25);
        storage->types[74].flags1=0;storage->types[74].flags2=0x400000;raw[40]|=1;core.factory.PostCreate(sid);CHECK(Get(raw,26)==25);
        globals->DaisType()=75;core.factory.PostCreate(sid);CHECK(Get(raw,26)==32);
        globals->DaisType()=74;priest.movement.priest=75;core.factory.PostCreate(sid);CHECK(Get(raw,26)==129 && globals->PriestType()==75);
    }
    CHECK(diagnostics==2 && storage->Generation()==2);globals->ResetForGeneralWorld();CHECK(globals->PriestType()==75);
}

TEST_CASE(RiftHitPointCurrent_MovementValueCopiesKeepDestinationBinding) { // 참조를 가진 실제 선택 상태의 독립 사본과 명시 대입 수명을 확인한다.
    RawGameAuxState globals;globals.PriestType()=0x8000014a;RawMovementProcessTypes bound(&globals.PriestType());
    std::optional<RawMovementProcessTypes> copy;
    { RawMovementProcessTypes temporary(bound);CHECK(&temporary.priest!=&bound.priest && temporary.priest==bound.priest);copy.emplace(temporary); }
    copy->priest=91;CHECK(globals.PriestType()==0x8000014a && copy->priest==91);
    const auto address=&bound.priest;bound=*copy;CHECK(&bound.priest==address && globals.PriestType()==91);
    bound=RawMovementProcessTypes{};CHECK(&bound.priest==address && globals.PriestType()==158);
}
