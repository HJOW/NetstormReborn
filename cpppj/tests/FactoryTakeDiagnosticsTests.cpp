// Take의 원문 진단 복귀와 실제 World/Session 상세 소유자를 창·게임·통신 없이 검사한다.
#include "TestSupport.h"
#include <filesystem>
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "o/SquidFactory.h"
#include "o/SquidUnpop.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

using namespace netstorm::o;
using namespace netstorm::client;
namespace { // 합성 입력은 원본 풀과 실제 생성/공간 해제 몸체에만 공급한다.
// 원문 진단 장치 누락과 물리 범위 실패를 성공으로 바꾸지 않는지 관측한다.
template<class F> bool TakeFailed(F&& operation) { try { operation(); } catch (const std::exception&) { return true; }return false; }
// 고정 입력의 두 TYPE만 사용하며 게임 자산이나 네트워크 객체를 생성하지 않는다.
struct TakeDiagnosticScene {
    SidPool pool{OriginalEdition::Patch1078,32768}; // 전체 물리 SID와 기존 자유 목록을 보유한다.
    SquidHash hash; // void Unpop이 보존해야 하는 실제 네 단계 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 별도 가상 지도 대신 실제 공간 배열을 쓴다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 진단 callback이 수정할 현재 TYPE 표다.
    SquidUnpop unpop{pool,hash,spots}; // Take가 호출하는 실제 공통 해제 부모다.
    SquidFactory factory{pool,types,false,&unpop}; // 생성자 뒤 현재 TYPE와 진단 장치를 연결한다.
    Sid source{},client{}; // 서버/클라이언트 영역의 실제 할당 번호다.
    std::optional<std::string> detail; // 원문 strdup처럼 입력 문구와 독립인 복사본이다.
    std::vector<std::uint32_t> lines; // 원문 assertion의 실행 순서다.
    std::vector<std::string> sequence; // 상세 복사와 assertion의 상대 순서를 관측한다.
    std::function<void(std::uint32_t)> onAssertion; // 진단 정상 복귀의 명시 외부 변경 입력이다.
    std::function<void()> onDetail; // 상세 writer가 실행된 뒤의 명시 외부 변경 입력이다.
    // 실제 진단은 현재 TYPE와 동일 물리를 빌리며 생성 자체에서는 호출되지 않는다.
    TakeDiagnosticScene() {
        types[74].name=std::string("SERVER\0tail",11);types[75].name="CLIENT";
        types[74].zOrder=17;types[75].zOrder=34;
        factory.BindDiagnostics({types,[this](auto expression,auto file,auto line) {
            CHECK(file=="\\Ns\\O\\Squid.cpp");CHECK(expression==(line==0x1a7 ? "(FIRST_SERVER_SID <= sid && sid < numSquids)" : "0"));
            lines.push_back(line);sequence.push_back("assert");if (onAssertion) onAssertion(line);
        },[this](auto text) { detail=std::string(text);sequence.push_back("detail");if (onDetail) onDetail(); }});
        source=factory.Create(74);client=factory.Create(74,2);unpop.BindDiagnostics({{},types,{}});
    }
    // 명시 변경은 같은 물리 슬롯에만 기록하며 풀 목록 API를 우회해 결과를 만들지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
};
}

TEST_CASE(FactoryTakeDiagnostics_SidAssertionResumesPhysicalClientSlot) { // 서버 범위 진단은 물리 범위 안 클라이언트 번호의 재개를 막지 않는다.
    TakeDiagnosticScene scene;const auto free=scene.pool.FreeCount();const auto first=scene.pool.FirstFree(true);const auto cursor=scene.pool.PredictableCursor();
    CHECK(scene.factory.Take(74,scene.client)==scene.client);
    CHECK(scene.lines==std::vector<std::uint32_t>{0x1a7} && !scene.detail && scene.Raw(scene.client)[10]==74 && scene.Raw(scene.client)[11]==4 && scene.Raw(scene.client)[35]==17);
    CHECK(scene.pool.FreeCount()==free && scene.pool.FirstFree(true)==first && scene.pool.PredictableCursor()==cursor);
}

TEST_CASE(FactoryTakeDiagnostics_OutOfPoolDiagnosesBeforeHostRangeFailure) { // 원문 진단은 먼저 실행하지만 풀 밖 메모리 접근은 재현하지 않는다.
    TakeDiagnosticScene scene;const auto before=std::vector(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
    CHECK(TakeFailed([&] { scene.factory.Take(74,Sid{32768}); }));
    CHECK(scene.lines==std::vector<std::uint32_t>{0x1a7} && !scene.detail && std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
    SquidFactory disconnected(scene.pool,scene.types);CHECK(TakeFailed([&] { disconnected.Take(74,scene.client); }));
    CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
}

TEST_CASE(FactoryTakeDiagnostics_MismatchCopiesNamesThenResumesCurrentConstructor) { // 상세→진단→실제 void Unpop→현재 생성자 순서를 유지한다.
    TakeDiagnosticScene scene;scene.Raw(scene.source)[10]=75;const auto free=scene.pool.FreeCount();const auto spots=scene.spots;
    scene.onDetail=[&] { CHECK(scene.Raw(scene.source)[10]==75 && scene.Raw(scene.source)[11]==4);scene.types[74].name="changed server";scene.types[75].name="changed client"; };
    scene.onAssertion=[&](auto line) { CHECK(line==0x1ba && scene.detail=="Invalid take. Server type SERVER, client type CLIENT");scene.types[74].constructorAddress=0x4794e0;scene.types[74].zOrder=0x123; };
    CHECK(scene.factory.Take(74,scene.source)==scene.source);const auto raw=scene.Raw(scene.source);
    CHECK(scene.lines==std::vector<std::uint32_t>{0x1ba} && scene.sequence==(std::vector<std::string>{"detail","assert"}));
    CHECK(raw[0]==8 && raw[1]==0xcc && raw[2]==0x50 && raw[3]==0 && raw[10]==74 && raw[11]==4 && raw[35]==0x23 && (raw[40]&2));
    CHECK(raw[26]==0 && raw[27]==0 && raw[28]==0 && raw[29]==0 && scene.pool.FreeCount()==free && scene.spots==spots);
}

TEST_CASE(FactoryTakeDiagnostics_CurrentDeviceCanRebindDuringBothCallbacks) { // 실행 중인 callable은 복사해 유지하고 다음 진단은 새 장치를 조회한다.
    TakeDiagnosticScene scene;scene.Raw(scene.source)[10]=75;std::uint32_t replacement{};
    scene.onDetail=[&] {
        scene.factory.BindDiagnostics({scene.types,[&](auto expression,auto file,auto line) {
            CHECK(expression=="0" && file=="\\Ns\\O\\Squid.cpp" && line==0x1ba);++replacement;
            scene.factory.BindDiagnostics({scene.types,{},{}});scene.types[74].zOrder=51;
        },{}});
    };
    CHECK(scene.factory.Take(74,scene.source)==scene.source);
    CHECK(replacement==1 && scene.lines.empty() && scene.detail=="Invalid take. Server type SERVER, client type CLIENT" && scene.Raw(scene.source)[35]==51);
}

TEST_CASE(FactoryTakeDiagnostics_SelectedMissingWritersPreservePriorEffects) { // 누락된 장치를 건너뛰지 않고 상세/진단 앞뒤의 부분 효과를 구별한다.
    TakeDiagnosticScene scene;scene.Raw(scene.source)[10]=75;const auto before=std::vector(scene.pool.Bytes().begin(),scene.pool.Bytes().end());std::uint32_t assertions{};
    SquidFactory noDetail(scene.pool,scene.types,false,&scene.unpop);noDetail.BindDiagnostics({scene.types,[&](auto,auto,auto) { ++assertions; },{}});
    CHECK(TakeFailed([&] { noDetail.Take(74,scene.source); }) && assertions==0 && !scene.detail);
    CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
    SquidFactory noAssertion(scene.pool,scene.types,false,&scene.unpop);noAssertion.BindDiagnostics({scene.types,{},[&](auto text) { scene.detail=std::string(text); }});
    CHECK(TakeFailed([&] { noAssertion.Take(74,scene.source); }));
    CHECK(scene.detail=="Invalid take. Server type SERVER, client type CLIENT" && std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
}

TEST_CASE(FactoryTakeDiagnostics_AssertionAbortKeepsCopiedDetailAndOldType) { // 실제 진단이 예외로 종료하면 복사한 상세만 남고 Unpop/생성자는 실행하지 않는다.
    TakeDiagnosticScene scene;scene.Raw(scene.source)[10]=75;const auto before=std::vector(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
    scene.onAssertion=[](auto) { throw std::runtime_error("명시 진단 종료"); };
    CHECK(TakeFailed([&] { scene.factory.Take(74,scene.source); }));
    CHECK(scene.detail=="Invalid take. Server type SERVER, client type CLIENT" && scene.lines==std::vector<std::uint32_t>{0x1ba});
    CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
}

TEST_CASE(FactoryTakeDiagnostics_ActualWorldsShareClientDetailOwner) { // 두 번 대여한 실제 World의 공장이 같은 Client 상세와 현재 TYPE를 사용한다.
    std::vector<RiftTypeRecord> types(188);types[74].name="server";types[75].name="client";
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));
    auto& detail=client.GameplayGlobals()->AiDebug().AssertionDetail(); // 실제 Client가 기존 표시 오류와 공유하는005e4798 소유자다. Run/창/네트워크는 실행하지 않는다.
    auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);std::uint32_t assertions{};
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="0" && file=="\\Ns\\O\\Squid.cpp" && line==0x1ba);CHECK(detail=="Invalid take. Server type server, client type client");++assertions;storage->types[74].zOrder=0x144; });
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    for (std::size_t generation=0;generation<2;++generation) { // World 반납 후에도 상세 owner를 Session 주소로 바꾸지 않는다.
        RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();const auto object=core.factory.Create(74);core.pool.AllocatedBytes(object)[10]=75;detail.reset();
        CHECK(core.factory.Take(74,object)==object && core.pool.Slot(object)[35]==0x44 && detail.has_value());
    }
    CHECK(assertions==2 && storage->Generation()==2);
}
