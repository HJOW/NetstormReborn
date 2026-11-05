// NotifySurface의 원문 관문·진단 복귀와 같은 현재 World 연결을 창 없이 대조한다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "RawSceneSupport.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawBuildingSpace.h"
#include <optional>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 원문 입력만 준비하며 진단 선택 결과를 대신 계산하지 않는다.
constexpr std::size_t kNotifyPrefix=128; // Native와 전체 바이트 체크섬을 비교하는 물리 슬롯 범위다.
std::uint32_t NotifyAdler(std::span<const std::uint8_t> bytes) { // zlib와 같은 Adler32로 범위 전체의 쓰기를 대조한다.
    std::uint32_t a=1,b=0;for (const auto byte:bytes) { a=(a+byte)%65521U;b=(b+a)%65521U; }return (b<<16)|a; // 원본 순서의 모든 BYTE를 소비한다.
}
std::string NotifyHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // 원문 ASCII 식/파일을 UTF 변환 없이 기록한다.
struct NotifyScene { // 같은 물리 슬롯과 현재 DWORD를 실제 몸체보다 오래 보관한다.
    SidPool pool; // 독립 client 풀의 범위 확인만 사용한다.
    SquidHash hash; // 옆면 caller와 실제 같은 표면 지도다.
    std::vector<RiftTypeRecord> types; // 초기 구성의 판본 길이이며 Notify는 TYPE 표를 조회하지 않는다.
    BuildingSpaceState state{97,82,false}; // 기존 독립 번호/진단 사본 계약이다.
    std::uint32_t bridge{82},checking{}; // 현재 TYPE와 전체 비영 checking DWORD 입력이다.
    RawBuildingSpace body; // 원문과 비교하는 실제 제품 몸체다.
    explicit NotifyScene(OriginalEdition edition):pool(edition,kCapacity,false),types(::netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171)),body(pool,hash,types,state) { body.BindCurrentBridgeType(bridge);body.BindNotifyDiagnostics(RawCheckingFlag{checking},{}); } // 대여 이후 각 입력이 현재값만 바꾼다.
    std::span<std::uint8_t> Raw(Sid sid=kSource) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free/예약 SID도 같은 물리를 쓴다.
    std::span<std::uint8_t> Prefix() { const auto bytes=pool.Bytes();return {const_cast<std::uint8_t*>(bytes.data()),kNotifyPrefix*pool.Layout().stride}; } // 이전 외부 상태를 다음 입력에 남기지 않는다.
    void Prepare(Sid sid,std::uint8_t type=82,std::uint16_t word=0xab7f) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;Put(raw,12,word,2);bridge=82;checking=6; } // 특정 객체 생성/행동을 실행하지 않는 물리 입력이다.
};
}

TEST_CASE(BuildingNotifyCurrent_Native2264RawTraceAndGlobals) { // 두 독립 원문 관측과 byte·prefix 전체·진단 문자열·복귀/현재 DWORD를 대조한다.
    const auto rows=LoadFixture(NETSTORM_BRIDGE_NOTIFY_FIXTURE).rows;std::size_t count{};
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 판본별 원문 주소와 필드 폭을 구별한다.
        NotifyScene scene(edition);const auto stride=scene.pool.Layout().stride;const auto outside=NotifyAdler(scene.pool.Bytes().subspan(kNotifyPrefix*stride));
        for (const auto& row:rows) { // 기존 관측 행을 재계산하거나 수정하지 않는다.
            if (row[0]!=(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")) continue;CHECK(row.size()==8);std::vector<std::uint32_t> input;
            for (const auto& value:Split(row[1],',')) input.push_back(Number(value)); // Native에 공급했던 정수만 읽는다.
            CHECK(input.size()==9);const Sid sid{static_cast<std::uint16_t>(input[0])};auto prefix=scene.Prefix();
            for (std::size_t index=0;index<prefix.size();++index) prefix[index]=static_cast<std::uint8_t>((index*29+7)&255); // Native와 같은 명시 물리 입력이다.
            auto raw=scene.Raw(sid);raw[10]=static_cast<std::uint8_t>(input[1]);raw[11]=static_cast<std::uint8_t>(input[2]);raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=static_cast<std::uint8_t>(input[3]);Put(raw,12,input[4],2);scene.bridge=input[5];scene.checking=input[6];std::vector<std::string> events;
            scene.body.BindNotifyDiagnostics(RawCheckingFlag{scene.checking},[&](auto expression,auto file,auto line) {
                events.push_back("X:"+NotifyHex(expression)+':'+NotifyHex(file)+':'+std::to_string(line));
                if (input[7]==1) { raw[10]=255;raw[11]=9;Put(raw,12,0xa1c2,2);raw[40]=8;scene.checking=0;scene.bridge=70; }
                else if (input[7]==2) { raw[10]=70;raw[11]=2;Put(raw,12,0x00a5,2);raw[40]=0x80;scene.checking=0x80000000U;scene.bridge=0xffffffffU; }
                if (!input[8]) throw std::runtime_error("명시 원문 진단 중단"); // 원문 진단 경계의 정상/중단 입력만 공급한다.
            });
            std::string outcome="return";try { scene.body.NotifySurface(sid); }catch (const std::exception&) { outcome="assertion"; }
            std::string trace;for (const auto& event:events) { if (!trace.empty()) trace+=';';trace+=event; } // 실제 호출 순서만 기록한다.
            const std::vector<std::string> actual{trace.empty() ? "-" : trace,Hex(raw),std::to_string(scene.checking),std::to_string(scene.bridge),outcome,std::to_string(NotifyAdler(prefix))};const std::vector<std::string> expected(row.begin()+2,row.end());
            if (actual!=expected) { std::printf("NotifySurface %s 입력%s\n",row[0].c_str(),row[1].c_str());CHECK(actual==expected);return; }++count;
        }
        CHECK(NotifyAdler(scene.pool.Bytes().subspan(kNotifyPrefix*stride))==outside);
    }
    CHECK(count==2264);
}
TEST_CASE(BuildingNotifyCurrent_OnlySelectedAssertionRequiresDevice) { // TYPE/extra/checking/nibble이 선택하지 않은 진단 장치는 요구하지 않는다.
    NotifyScene scene(OriginalEdition::Patch1078);scene.Prepare(kSource);CHECK(Throws([&] { scene.body.NotifySurface(kSource); }));scene.checking=0;scene.body.NotifySurface(kSource);scene.checking=0x80000000U;scene.Raw()[40]=8;scene.body.NotifySurface(kSource);scene.Raw()[40]=0;scene.bridge=338;scene.body.NotifySurface(kSource);scene.bridge=82;scene.Raw()[12]=0x3f;scene.body.NotifySurface(kSource);
    std::uint32_t calls{};scene.bridge=255;scene.Raw()[10]=255;scene.Raw()[12]=0x40;scene.body.BindNotifyDiagnostics(RawCheckingFlag{scene.checking},[&](auto expression,auto file,auto line) { CHECK(expression=="num >= 0 && num <= 7" && file=="\\Ns\\O\\Bridge.cpp" && line==0xff);++calls; });scene.body.NotifySurface(kSource);CHECK(calls==1 && scene.checking==0x80000000U);
}
TEST_CASE(BuildingNotifyCurrent_CurrentAndIndependentTypeContracts) { // 독립 상태는 기존 값 참조를 유지하고 명시 대여 뒤에는 현재 DWORD가 우선한다.
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false);SquidHash hash;std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);BuildingSpaceState state{97,82,true};RawBuildingSpace body(pool,hash,types,state);auto raw=const_cast<std::uint8_t*>(pool.Slot(kSource).data());raw[10]=82;raw[12]=0x7f;raw[40]=0;std::uint32_t calls{},bridge=338;body.BindNotifyDiagnostics(state.debug,[&](auto,auto,auto) { ++calls; });
    body.NotifySurface(kSource);state.bridgeType=83;body.NotifySurface(kSource);CHECK(calls==1);body.BindCurrentBridgeType(bridge);state.bridgeType=82;body.NotifySurface(kSource);CHECK(calls==1);bridge=82;body.NotifySurface(kSource);CHECK(calls==2);
}
TEST_CASE(BuildingNotifyCurrent_RebindingKeepsInvokedCallableAlive) { // 진단 중 자신의 연결을 반납해도 실행 중인 callable은 복귀까지 살아 있다.
    NotifyScene scene(OriginalEdition::Patch1078);scene.Prepare(kSource);auto token=std::make_shared<int>(7);std::weak_ptr<int> lifetime=token;std::uint32_t calls{};
    scene.body.BindNotifyDiagnostics(RawCheckingFlag{scene.checking},[&,keep=token](auto,auto,auto) { CHECK(*keep==7);++calls;scene.body.BindNotifyDiagnostics(RawCheckingFlag{scene.checking},{});CHECK(!lifetime.expired()); });token.reset();scene.body.NotifySurface(kSource);CHECK(calls==1 && lifetime.expired());CHECK(Throws([&] { scene.body.NotifySurface(kSource); }));
}
TEST_CASE(BuildingNotifyCurrent_SideWritesPrecedeReturningDiagnostic) { // 실제 옆면 caller의 BYTE 쓰기는 진단 전에 끝나고 정상 복귀 뒤 보존된다.
    NotifyScene scene(OriginalEdition::Patch1078);scene.Prepare(kSource);scene.types[82].flags1=TypeFlag1::kSurface;scene.hash.Cell(0,10,20)=kSource.value;std::uint32_t calls{};
    scene.body.BindNotifyDiagnostics(RawCheckingFlag{scene.checking},[&](auto,auto,auto) { CHECK(scene.Raw()[12]==0x79);++calls; });scene.body.PopSide(10,20,40,40);CHECK(scene.Raw()[12]==0x79 && calls==1);
    scene.Raw()[12]=0xff;scene.body.BindNotifyDiagnostics(RawCheckingFlag{scene.checking},[&](auto,auto,auto) { CHECK(scene.Raw()[12]==0xf9);++calls;scene.checking=0; });scene.body.UnpopSide(10,20);CHECK(scene.Raw()[12]==0xf9 && calls==2 && scene.checking==0);
}
TEST_CASE(BuildingNotifyCurrent_DiagnosticFailurePreservesSideAndCallbackWrites) { // 진단이 변경 후 중단해도 앞선 실제 옆면/콜백 쓰기를 되돌리지 않는다.
    NotifyScene scene(OriginalEdition::Patch1078);scene.Prepare(kSource);scene.hash.Cell(0,10,20)=kSource.value;scene.body.BindNotifyDiagnostics(RawCheckingFlag{scene.checking},[&](auto,auto,auto) { CHECK(scene.Raw()[12]==0x79);scene.Raw()[12]=0x21;scene.Raw()[11]=9;throw std::runtime_error("명시 지면 진단 중단"); });CHECK(Throws([&] { scene.body.UnpopSide(10,20); }) && scene.Raw()[12]==0x21 && scene.Raw()[11]==9);
}
TEST_CASE(BuildingNotifyCurrent_CdDoesNotDereferenceUnusedSidOrDevice) { // CD 호출자가 버리는 포인터 계산은 raw나 진단 장치를 읽지 않는다.
    NotifyScene scene(OriginalEdition::Cd1072);scene.body.BindNotifyDiagnostics(RawCheckingFlag{scene.checking},[](auto,auto,auto) { throw std::logic_error("CD의 미도달 진단"); });scene.body.NotifySurface(Sid{65000});
}
TEST_CASE(BuildingNotifyCurrent_ActualWorldUsesCurrentTypeCheckingAndClientDevice) { // 장기 Storage를 빌리는 실제 World도 TYPE/현재 checking/진단 장치 교체를 바로 소비한다.
    using namespace netstorm::client;std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames(std::vector<FrameCode>{}));std::vector<SquidDisplayShape> shapes(188);BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.noIslandType=157;links.connectorType=155;
    std::int32_t checking{};std::optional<std::string> detail;std::uint32_t calls{};auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);storage->BindChecking(RawCheckingFlag{checking});storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="num >= 0 && num <= 7" && file=="\\Ns\\O\\Bridge.cpp" && line==0xff);++calls; });
    { RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,links,97,nullptr,storage);auto core=world.Core();const auto sid=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(sid);raw[10]=74;raw[12]=0x7f;raw[40]=0;core.bridgeState.bridgeType=74;core.building.NotifySurface(sid);CHECK(calls==0);checking=std::bit_cast<std::int32_t>(0x80000000U);core.building.NotifySurface(sid);CHECK(calls==1);
      storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="num >= 0 && num <= 7" && file=="\\Ns\\O\\Bridge.cpp" && line==0xff);calls+=2; });core.building.NotifySurface(sid);CHECK(calls==3 && checking==std::bit_cast<std::int32_t>(0x80000000U)); }
    CHECK(!storage->Borrowed());
}
