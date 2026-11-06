// PATCH 실제 할당/단언 복귀/최대50회수와 Native Release의 전체 상태를 창 없이 대조한다.
#include "TestSupport.h"
#include "FormTypeInputs.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawProcessHostStorage.h"
#include "o/SquidFactory.h"
#include "o/SquidForms.h"
#include "o/RawGameAuxState.h"
#include "o/ContentFormLifetime.h" // 실제 Session이 World에 후설치하는 Content 가상 호스트를 같은 방식으로 연결한다.
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace { // 원문 입력/장치 경계와 전체 풀/삭제 기록의 체크섬만 준비한다.
using namespace netstorm::o;using namespace netstorm::client;
constexpr std::uint32_t kCapacity=23005,kFakeTable=0x10001000; // Native 입력의합법Pool크기와가상삭제경계VT기록값이다.
// 텍스트 사건은 단언 식/경로의 실제 바이트를 잃지 않는16진수로 비교한다.
std::string RecoveryHex(std::string_view text) {
    constexpr char digits[]="0123456789abcdef"; // 각 반 BYTE를 표현하는 고정 문자표다.
    std::string result;
    for (const unsigned char value:text) { // 원문 문자열의 각 BYTE를 두 문자로 보존한다.
        result+=digits[value>>4];result+=digits[value&15];
    }
    return result;
}
// 독립 Native zlib 결과와 전체 바이트를 비교하는 표준Adler32다.
std::uint32_t RecoveryAdler(std::span<const std::uint8_t> bytes) {
    std::uint64_t a=1,b=0;std::size_t count{};
    for (const auto value:bytes) { // 전체 바이트를 순서대로 누적하고 중간 넘침 전에 나머지를 구한다.
        a+=value;b+=a;if (++count==4096) { a%=65521;b%=65521;count=0; }
    }
    return static_cast<std::uint32_t>(((b%65521)<<16)|(a%65521));
}
// 명시 입력/원문 BYTE 효과만 같은 물리 슬롯에 쓰며 할당 목록은 별도 입력으로 둔다.
std::span<std::uint8_t> RecoveryRaw(SidPool& pool,std::uint32_t sid) { const auto bytes=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
// 정렬되지 않은 WORD/DWORD를 인접 필드 변경 없이 기록한다.
void RecoveryPut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8)); // 지정한 폭의 낮은 BYTE부터 기록한다.
}
// 두 삭제 기록 배열을 원문client/other 순서의 little endian320BYTE로 읽는다.
std::uint32_t RecoveryLogAdler(const SidPool& pool) {
    std::vector<std::uint8_t> bytes;
    for (const bool client:{true,false}) { // 원문 client 기록 뒤 other 기록을 이어 붙인다.
        for (const auto& record:pool.Deletions(client)) { // 각 배열의 최신 기록부터20개를 읽는다.
            for (const auto value:{record.sid,record.type}) { // SID와 TYPE 두 DWORD를 패딩 없이 직렬화한다.
                for (std::size_t i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(i*8))); // DWORD의 낮은 BYTE부터 옮긴다.
            }
        }
    }
    return RecoveryAdler(bytes);
}
// 진단/가상 삭제의 외부 효과는 Native 도구와 같은 명시 입력이며 실제 반납은 제품 API를 사용한다.
struct RecoveryScene {
    std::uint32_t mode,flags; // 입력 오염/장치 효과와 원문 할당 플래그다.
    bool server,boss; // 서로다른현재권한주소이며콜백뒤에도사본으로고정하지않는다.
    std::uint32_t bridge{82},disallow{},kills{}; // 현재전체DWORD종류/금지값과실제삭제진입횟수다.
    SidPool pool; // 장치가빌리는모든전역보다먼저반납되는실제Pool이다.
    std::string events; // 원문이선택한단언/삭제의동기호출순서다.
    RecoveryScene(std::uint32_t selected,std::uint32_t allocation,bool isServer,bool isBoss):mode(selected),flags(allocation),server(isServer),boss(isBoss),pool(OriginalEdition::Patch1078,kCapacity,server,&server) { // 준비할당뒤명시 raw입력을적용한다.
        if (mode==15) { pool.Allocate(1);pool.Allocate(1); }
        const bool client=(flags&2U)!=0;const auto tail=pool.Tail(client);const auto head=pool.FirstFree(client);if (mode==1) disallow=1;
        if (mode==3) RecoveryRaw(pool,head.value)[11]=0;
        if (mode==4 || (mode>=5 && mode<=9) || mode==17) RecoveryPut(RecoveryRaw(pool,0),client ? 4:18,tail.value,2);
        if (mode==6) { for (std::uint32_t sid=10;sid<70;++sid) SetCandidate(sid,82); } // 원문50회수상한뒤10개후보가남아야한다.
        else if (mode>=5 && mode<=9) { SetCandidate(10,82);SetCandidate(11,mode==7 ? 70:82);SetCandidate(13,82); }
        if (mode==17) bridge=0x10052;
        if (mode==11) RecoveryRaw(pool,23001)[11]=0;
        if (mode==12) RecoveryRaw(pool,23002)[11]=0;
        if (mode==13 || mode==14) RecoveryPut(RecoveryRaw(pool,23002),4,mode==13 ? 23004:23001,2);
        pool.BindAllocation({&disallow,&boss,&bridge,[this](auto expression,auto file,auto line) { Add("A:"+std::to_string(line)+":"+RecoveryHex(expression)+":"+RecoveryHex(file));if (mode==1 && line==0xe8) disallow=0;if (mode==11 && line==0xf0) RecoveryRaw(pool,23001)[11]=1; },[this](Sid sid) { Reclaim(sid); }});
    }
    void Add(std::string text) { if (!events.empty()) events+=';';events+=text; } // 사건구분자는Native파일경계와같다.
    void SetCandidate(std::uint32_t sid,std::uint8_t type) { auto raw=RecoveryRaw(pool,sid);RecoveryPut(raw,0,kFakeTable);raw[10]=type;raw[11]=4; } // 실제Native Release가받는void/live입력이다.
    void Reclaim(Sid sid) { ++kills;Add("K:"+std::to_string(sid.value)+":0");pool.Release(sid);if (kills==1) { if (mode==7) bridge=70;else if (mode==8) SetCandidate(12,82);else if (mode==9) throw std::runtime_error("명시 삭제 장치 실패"); } } // 반납뒤현재종류/다음후보를바꾸거나예외를전달한다.
};
// 실제 World가 같은 긴 물리를 재대여할 때 사용하는 기본 연결 번호다.
BridgeConnectState RecoveryLinks() { BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;return links; }
}

TEST_CASE(SidAllocationRecovery_Native47CurrentDiagnosticsListsAndRelease) { // 반환값/전체Pool/기록/현재전역/호출순서를독립Native47행과비교한다.
    std::ifstream input(NETSTORM_SID_ALLOCATION_RECOVERY_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{};
    while (std::getline(input,line)) { // 모든동결입력과가상장치예외의부분상태를누락없이읽는다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;std::stringstream stream(line);std::vector<std::string> fields;std::string field;
        while (std::getline(stream,field,'\t')) fields.push_back(field); // 같은 행의 명시 입력/관측17열을 원문 순서로 분리한다.
        CHECK(fields.size()==17);if (fields.size()!=17) continue;
        const auto number=[&](std::size_t index) { return static_cast<std::uint32_t>(std::stoul(fields[index])); };RecoveryScene scene(number(0),number(1),number(2)!=0,number(3)!=0);bool stopped=false;std::uint32_t result{};try { result=scene.pool.Allocate(scene.flags).value; }catch (const std::exception&) { stopped=true; }
        CHECK(stopped==(number(4)!=0));const std::array<std::uint32_t,9> actual{result,scene.pool.FreeCount(),scene.pool.PredictableCursor(),scene.pool.FirstFree(true).value,scene.pool.Tail(true).value,scene.pool.FirstFree(false).value,scene.pool.Tail(false).value,RecoveryAdler(scene.pool.Bytes()),RecoveryLogAdler(scene.pool)};
        for (std::size_t i=0;i<actual.size();++i) CHECK(actual[i]==number(5+i)); // 전체메모리를Native체크섬에대조하고제품계산을기대값으로사용하지않는다.
        CHECK(scene.bridge==number(14) && scene.disallow==number(15));CHECK((scene.events.empty() ? "-":scene.events)==fields[16]);++rows;
    }CHECK(rows==47);
}

TEST_CASE(SidAllocationRecovery_ActualWorldReclaimsCurrentClassThroughLongPool) { // 긴Pool이현재World의실제가상분배를사용하고다음대여에서옛주소를읽지않는다.
    auto types=netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr);std::optional<std::string> detail;std::size_t assertions{};storage->BindDisplayDiagnostics(detail,[&](auto,auto,auto) { ++assertions; });
    {
        RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,frames,shapes,RecoveryLinks(),83,nullptr,storage);auto core=world.Core();
        ContentLifetimeState state;ContentFormHost contents(core.pool,core.types,[](auto,auto,auto) {});ContentFormLifetime lifetime(core.pool,core.types,contents,core.destroy,state,{},core.forms.Hooks());core.forms.Bind(lifetime); // 실제 Session과 같이 Content 호스트를 World 생성 뒤 후설치한다.
        const auto first=core.factory.CreateContained(2);const auto before=core.pool.FreeCount();RecoveryPut(RecoveryRaw(core.pool,0),4,core.pool.Tail(true).value,2);
        storage->links.bridgeType=0x10006;bool wideRejected=false;storage->BindDisplayDiagnostics(detail,[&](auto,auto,auto line) { CHECK(line==0x14a);throw std::logic_error("전체 DWORD 비교 뒤 예약 꼬리 진단"); });
        try { core.pool.Allocate(2); }catch (const std::logic_error&) { wideRejected=true; }CHECK(wideRejected && !(core.pool.Slot(first)[11]&1U) && core.pool.FreeCount()==before); // 원문 DWORD 비교를 BYTE로 줄이면 앞의 live Content가 잘못 회수된다.
        storage->links.bridgeType=6;storage->BindDisplayDiagnostics(detail,[&](auto,auto,auto) { ++assertions; });const auto replacement=core.factory.CreateContained(2);
        CHECK(first.value==5 && replacement.value==14999 && (core.pool.Slot(first)[11]&1U));CHECK(core.pool.FreeCount()==before && core.pool.FirstFree(true)==first && core.pool.Tail(true)==first && !assertions);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);CHECK(world.Destroy(replacement,0x10));
        const auto residual=core.factory.CreateContained(2);CHECK(residual.value==first.value);auto pass=world.ClearProcessesForState();world.ReturnProcessBindingsForState(std::move(pass)); // 이잔류를다음prepared대여까지물리에남긴다.
    }
    storage->links.bridgeType=6;RecoveryPut(RecoveryRaw(storage->pool,0),4,storage->pool.Tail(true).value,2);bool failed=false;try { storage->pool.Allocate(2); }catch (const std::logic_error&) { failed=true; }CHECK(failed); // 종료된가상대여의선택은명시적으로거부한다.
    {
        RawSurfaceWorld next(OriginalEdition::Patch1078,storage->types,frames,shapes,RecoveryLinks(),83,nullptr,storage,true);storage->links.bridgeType=6;auto core=next.Core();
        ContentLifetimeState state;ContentFormHost contents(core.pool,core.types,[](auto,auto,auto) {});ContentFormLifetime lifetime(core.pool,core.types,contents,core.destroy,state,{},core.forms.Hooks());core.forms.Bind(lifetime); // 새 대여도 같은 현재 클래스의 새 호스트만 사용한다.
        const auto value=core.factory.CreateContained(2);CHECK(value.value && !assertions && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);CHECK(next.Destroy(value,0x10));
    }
}

TEST_CASE(SidAllocationRecovery_SelectedMissingReclaimKeepsPhysicalPrefix) { // 회수장치누락은고정된사전검사로숨기지않고선택된live후보에서실패한다.
    RecoveryScene scene(5,2,true,true);scene.pool.BindAllocation({&scene.disallow,&scene.boss,&scene.bridge,[](auto,auto,auto) {},{}});const auto before=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());const auto free=scene.pool.FreeCount();bool failed=false;try { scene.pool.Allocate(2); }catch (const std::logic_error&) { failed=true; }CHECK(failed && scene.pool.FreeCount()==free && std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
}

TEST_CASE(SidAllocationRecovery_ReclaimPinsBindingUntilSynchronousReturn) { // 회수 콜백이 지난 Clear 증빙으로 현재 대여를 반납하지 못하고 복귀 뒤에만 반납한다.
    auto authority=std::make_shared<RawAuthorityState>();SidPool pool(OriginalEdition::Patch1078,kCapacity,true,&authority->server);auto types=netstorm::test::MakeFormTypeInputs(188);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);RawProcessHostStorage storage(pool,types,authority);
    auto runtime=std::make_shared<ProcessHostRuntimeBindings>();runtime->destroy=&destroy;auto use=storage.Bind(runtime);auto pass=use->ClearForState();const auto first=pool.Allocate(2);RecoveryRaw(pool,first.value)[10]=82;std::uint32_t disallow{},bridge=82,calls{};bool rejected=false;
    runtime->asset.emit=[&](const SquidDestroyEvent& event) { // 현재 효과의 외곽 대여 수가 실제 가상 삭제의 정상/예외 복귀까지 유지돼야 한다.
        CHECK(event.effect==SquidDestroyEffect::DestroyDependent && event.sid==first && event.flags==0 && runtime->activeCalls==1);++calls;
        try { use->ReturnForState(std::move(pass)); }catch (const std::logic_error& error) { rejected=std::string_view(error.what()).find("진행 중")!=std::string_view::npos; }
        pool.Release(event.sid);
    };
    pool.BindAllocation({&disallow,&authority->boss,&bridge,[](auto,auto,auto) {},[&](Sid sid) { storage.ReclaimForAllocation(sid); }});RecoveryPut(RecoveryRaw(pool,0),4,pool.Tail(true).value,2);const auto replacement=pool.Allocate(2);
    CHECK(replacement.value==14999 && calls==1 && rejected && runtime->activeCalls==0);auto returned=use->ReturnForState(std::move(pass));CHECK(runtime->activeCalls==0); // 같은 증빙은 거부 때 소비하지 않고 호출이 끝난 뒤 소비한다.
}
