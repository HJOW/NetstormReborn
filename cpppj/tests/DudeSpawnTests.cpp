// 실제 보호 Dude65클러스터/130물리프레임과 같은 factory/owner/Pop/Frame/장부를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDudeSpawn.h"
#include "o/SquidFrameBinding.h"
#include "client/SquidRenderer.h"
#include "client/Renderer.h"
#include <cstdio>
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 기대 출력은 독립 실제 PE가 저장한 fixture에만 있다.
const std::vector<std::vector<std::string>>& SpawnRows() { static const auto rows=LoadFixture(NETSTORM_DUDESPAWN_FIXTURE).rows;return rows; }
// 양판본 보호 자산을 한 번씩 읽어 검사 장면의 현재 자료를 공급한다.
struct DudeAssets {
    std::filesystem::path root; // 판본의 보호 archive 경로이며 BaseFileSystem과 같은 입력이다.
    BaseFileSystem files; // 보호 디렉터리/TAFF를 읽는 기존 실제 자산 저장소다.
    netstorm::client::GameAssets game; // 실제 Dude TYPE/SHP/팔레트를 해석한 현재 자산이다.
    std::vector<RiftTypeRecord> types; // 원본 로딩 번호와 실제 생성자 주소의 타입 표다.
    std::vector<RiftTypeFrames> frames; // 실제 Dude I00/64를 포함하는 현재 Run 표다.
    std::vector<SquidDisplayShape> shapes; // 같은130물리프레임의 실제 크기/핫스폿이다.
    // 같은 보호 자산을 읽기 전용으로 연결하며 창/장치를 만들지 않는다.
    explicit DudeAssets(OriginalEdition edition):root(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")),files(root),game(Register(),edition),types(game.TypeTable().Types().begin(),game.TypeTable().Types().end()),frames(types.size(),RiftTypeFrames({})),shapes(netstorm::client::SquidRenderer::Shapes(game,edition)) {
        // 물리 블록의 자산 번호70부터 실제 프레임 Run을 채운다.
        for (const auto& asset:game.Types()) frames[70+asset.block]=asset.definition.FrameTable();
        CHECK(types[70].footX==0 && types[70].footY==0 && types[70].maxHitPoints==20 && frames[70].Run('I').first==64 && shapes[70].frameCount==65);
        // 원본 준비용 미등록 base74는 합성 빈 타입이며 새 Dude의 보호 자료와 구분한다.
        types[74]={};
    }
    // 초기화 순서상 GameAssets 전에 같은 archive를 실제 등록한다.
    const BaseFileSystem& Register() { files.RegisterArchive(root/"netstorm.tarc");return files; }
};
// 판본마다 보호 자산을 한 번 읽으며 반복 행에서 원본 입력으로 재사용한다.
const DudeAssets& Assets(OriginalEdition edition) { static const DudeAssets patch(OriginalEdition::Patch1078),cd(OriginalEdition::Cd1072);return edition==OriginalEdition::Patch1078 ? patch : cd; }
// 원본 표시 변경 표와 같은 병합/전체 갱신 상태를 보관한다.
struct DudeSink final:SquidDisplaySink {
    netstorm::client::DirtyRegions dirty{1024,768}; // 원본 viewport의 실제 변경 영역 저장소다.
    // 전체 갱신 이후의 표시 생략 상태다.
    bool Suppressed() const override { return dirty.FullRedraw(); }
    // 원본 main/shadow 순서로 같은 실제 병합기에 추가한다.
    void Invalidate(SquidDisplayRect rectangle,std::uint32_t flags) override { dirty.Add({rectangle.left,rectangle.top,rectangle.right,rectangle.bottom},flags); }
};
// WORD/DWORD 표의 체크섬 입력을 원본 little endian으로 직렬화한다.
template<class T> std::uint32_t WordAdler(std::span<const T> words) {
    std::vector<std::uint8_t> bytes;
    // 호스트의 정렬/바이트 순서와 무관하게 저장 비트를 기록한다.
    for (const auto word:words) for (std::size_t index=0;index<sizeof(T);++index) bytes.push_back(static_cast<std::uint8_t>(word>>(index*8)));
    return Adler(bytes);
}
// 같은 원본 상태와 실제 모든 생성/공간/표시/후처리 처리기를 소유한다.
struct SpawnScene {
    SidPool pool; // 실제 Reset의 서버 저장소이며 client flags2로 Dude6을 생성한다.
    SquidHash hash; // 실제 Pop/Frame의 같은 네 단계 머리 목록이다.
    std::vector<std::uint8_t> spots; // 발자국0의 빈 순회가 보존하는 전체65536바이트다.
    const DudeAssets& assets; // 실제 타입/Run/SHP의 읽기 전용 공유 자료다.
    SquidPostPopState bookkeeping; // 원본 세 통계 표와 공통 깊이/비용이다.
    SquidPostPopList population; // 실제 Dude 등록 목록의 capacity/count/stale 값이다.
    DudeSpawnState spawnState; // 생성/최초 Pop이 공유하는 현재 population과 blocked 전역이다.
    DudeSink sink; // 실제 표시가 갱신한 원본 viewport 변경 표다.
    SquidDisplay display; // 실제 보호 SHP의 프레임/그림자 영역 계산기다.
    SquidUnpop unpop; // 실제 Frame 재등록 경로의 동일 공간 처리기다.
    SquidPostPop post; // 같은 공통 장부와 가상 후처리 분배기다.
    SquidPop pop; // 같은 풀/해시/spot/display의 실제 등록 몸체다.
    SquidFactory factory; // 실제 SID 할당/Dude 생성자/postCreate 처리기다.
    SquidOwnerMode ownerMode; // 원본의 비전투/비요새 소유자 모드다.
    SquidOwner owner; // 실제 공통 owner 가상 몸체다.
    SquidFrame frame; // 실제 I00 변경/공간/표시 요청을 수행한다.
    RawDudePostPop dudePost; // 실제 task5101/population/I첫프레임/공통 body다.
    // 같은 풀과 모든 실제 처리기를 구성한 뒤 현재 입력만 채운다.
    explicit SpawnScene(const std::vector<std::string>& row):pool(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,true),spots(65536),assets(Assets(pool.Edition())),spawnState{population,Number(row[3])!=0},display(pool.Edition(),assets.types,assets.shapes,sink,{0,0,65536,{0,0,1024,768}}),unpop(pool,hash,spots,&display),post(pool,assets.types,bookkeeping),pop(pool,hash,spots,&display,&post),factory(pool,assets.types,false,&unpop),owner(pool,assets.types,bookkeeping,ownerMode),frame(pool,assets.types,MakeSquidFrameHooks(pool,assets.types,display,unpop,pop)),dudePost(pool,assets.frames,spawnState,MakeDudePostPopHooks(pool,frame,post)) {
        CHECK(factory.Create(74,2)==Sid{5});population.entries.resize(Number(row[1]));population.count=Number(row[2]);bookkeeping.localOwner=1;
        // 원본 전체 배열의 현재 stale DWORD 입력을 그대로 만든다.
        for (std::size_t index=0;index<population.entries.size();++index) population.entries[index]=0x60000000U+static_cast<std::uint32_t>(index);
        const std::array<std::span<std::uint32_t>,3> tables{bookkeeping.localCounts,bookkeeping.localSecondaryCounts,bookkeeping.globalCounts};
        // 원본 seed0과 각 표의 서로 다른17 오프셋을 유지한다.
        for (std::size_t table=0;table<tables.size();++table) for (std::size_t index=0;index<tables[table].size();++index) tables[table][index]=static_cast<std::uint32_t>(index)*2654435761U+static_cast<std::uint32_t>(table)*17U;
        post.SetDudePostPop(&dudePost);
    }
    // 전체 raw/공간/장부/변경 표 결과를 원본 순서로 직렬화한다.
    std::vector<std::string> Run(const std::vector<std::string>& row) {
        DudeSpawnHooks hooks;hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { const auto raw=pool.Slot(sid);const auto size=display.FrameSize(pool.Edition(),raw,0);if (pop.Pop(sid,assets.types[raw[10]],size[0],size[1],x,y,flags)!=RawPopResult::Registered) throw std::logic_error("Dude 실제 Pop 등록 실패"); };RawDudeSpawn spawn(pool,factory,owner,spawnState,std::move(hooks));const auto born=spawn.Spawn(std::bit_cast<float>(Number(row[4])),std::bit_cast<float>(Number(row[5])),Number(row[6]));
        if (born.value && Number(row[7])) dudePost.PostPop(born,0x50);
        std::string populationText,counts="0,0",rectangles;
        // capacity 전체를 비교하며 count 밖 stale 값도 보존한다.
        for (const auto entry:population.entries) { if (!populationText.empty()) populationText+=',';populationText+=std::to_string(entry); }
        if (populationText.empty()) populationText="-";
        std::vector<std::uint8_t> heads;
        // 원본 네 버킷 배열 전체를 low WORD 바이트로 기록한다.
        for (int level=0;level<4;++level) for (const auto value:hash.Entries(level)) { heads.push_back(static_cast<std::uint8_t>(value));heads.push_back(static_cast<std::uint8_t>(value>>8)); }
        // 현재 owner 목록은 모두 빈 입력이며 실제 count를 각각 기록한다.
        for (const auto& list:bookkeeping.ownerFactories) counts+=','+std::to_string(list.count);
        // 실제 원본 main/shadow 변경 영역과 flags의 순서를 보존한다.
        for (const auto& entry:sink.dirty.Entries()) { if (!rectangles.empty()) rectangles+=';';rectangles+=std::to_string(entry.rect.left)+','+std::to_string(entry.rect.top)+','+std::to_string(entry.rect.right)+','+std::to_string(entry.rect.bottom)+','+std::to_string(entry.flags); }
        if (rectangles.empty()) rectangles="-";const auto raw=Hex(pool.Slot(born.value ? born : Sid{6}));
        return {std::to_string(born.value),std::to_string(population.count),populationText,raw,std::to_string(Adler(heads)),std::to_string(Adler(spots)),std::to_string(bookkeeping.totalCost),std::to_string(bookkeeping.productionDirty),std::to_string(bookkeeping.depth),std::to_string(WordAdler<std::uint32_t>(bookkeeping.localCounts)),std::to_string(WordAdler<std::uint32_t>(bookkeeping.localSecondaryCounts)),std::to_string(WordAdler<std::uint32_t>(bookkeeping.globalCounts)),counts,"1",raw,sink.Suppressed() ? "1" : "0",rectangles};
    }
};
// 각 판본의 독립 원본 입력/출력을 실제 처리기로 전수 재생한다.
void ReplaySpawn(std::string_view edition) {
    std::size_t count=0;int failures=0;
    // 기대 칸은 실제 실행 후 비교에만 사용한다.
    for (const auto& row:SpawnRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==25);SpawnScene scene(row);const auto output=scene.Run(row);const auto expected=std::vector<std::string>(row.begin()+8,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 첫 차이는 원본 값과 C++ 값을 함께 남긴다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s DudeSpawn 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==144);
}
}
// 실제10.78 factory/owner/Pop/Frame/후처리와 capacity/차단을 대조한다.
TEST_CASE(DudeSpawn_PatchNativeCreationAndInitialPop) { ReplaySpawn("originals"); }
// CD10.72의 frame BYTE와 실제130물리프레임/빈 발자국 순회를 대조한다.
TEST_CASE(DudeSpawn_CdNativeCreationAndInitialPop) { ReplaySpawn("originalCD"); }

namespace { // 현재 목록/물리 슬롯/타입 Run과 실제 Factory/Owner/공통 부모를 조립한다.
struct DudeCurrentScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true};std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 같은 PATCH 풀과 현재 TYPE 표다.
    SquidPostPopList population;DudeSpawnState state{population}; // 최초 등록/생성이 공유하는 현재 목록과 전역이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); // 현재 TYPE+150의 I 첫 프레임 입력이다.
    SquidPostPopState book;SquidPostPop base{pool,types,book};SquidOwnerMode mode; // 같은 공통 깊이/장부와 Owner 모드다.
    SquidFactory factory{pool,types};SquidOwner owner{pool,types,book,mode}; // Spawn의 실제 생성자/소유자 쓰기를 재사용한다.
    std::span<std::uint8_t> raw;std::vector<std::string> events; // 대상5의 현재 뷰와 실제 도달 경계의 호출 관찰이다.
    DudeCurrentScene() { CHECK(pool.Allocate(2)==Sid{5});raw=pool.AllocatedBytes(Sid{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x508cc8);raw[10]=74;raw[34]=1;Put(raw,12,0xbeef,2);frames[74]=RiftTypeFrames({{'A','A',0,0},{'I','A',0,0}});frames[75]=RiftTypeFrames({{'A','A',0,0},{'A','A',1,0},{'I','A',0,0}});population.entries={0xaaaa0000,0xbbbb0000};book.suppressed=true;book.depth=1;base.BindCurrentTypes(types); } // 출력은 계산하지 않고 현재 raw/프레임/목록의 입력만 준비한다.
    DudePostPopHooks Hooks() { // Frame 장치의 인자와 공통 부모만 관찰하며 제품의 QA/목록 쓰기를 대신하지 않는다.
        DudePostPopHooks hooks;hooks.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) { CHECK(sid==Sid{5} && flags==0);events.push_back("F"+std::to_string(frame));Put(raw,36,std::bit_cast<std::uint32_t>(frame)); };hooks.validateBase=[](auto,auto) { throw std::logic_error("별도 사전 검사"); };hooks.base=[this](Sid sid,auto flags) { events.push_back("B");base.PostPopBase(sid,flags); };return hooks;
    }
};
}
TEST_CASE(DudeParentCurrent_NoFirstSkipsRawPopulationAndFrameDevices) { // 실제 몸체의 flags0/2/4는 물리 슬롯/목록/프레임 장치 없이 공통 부모만 실행한다.
    for (const auto flags:{0U,2U,4U}) { // 최초 비트가 없는 독립 입력은 잘못된 SID도 몸체 안에서 먼저 읽지 않는다.
        DudeCurrentScene scene;auto hooks=scene.Hooks();hooks.setFrame={};RawDudePostPop body(scene.pool,{},scene.state,std::move(hooks));body.PostPop(Sid{24000},flags);CHECK(scene.events==std::vector<std::string>{"B"} && scene.book.depth==0 && scene.population.count==0 && Get(scene.raw,12,2)==0xbeef);
    }
}
TEST_CASE(DudeParentCurrent_ActualVtableAcceptsDifferentTypeAndPhysicalSlot) { // PATCH VT로 선택한 free/contained 슬롯도 QA/등록/현재 Run/공통 부모를 원문 순서로 실행한다.
    DudeCurrentScene scene;scene.raw[11]=1;scene.raw[40]=8;RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks());scene.base.SetDudePostPop(&body);CHECK(body.Handles(Sid{5}) && Throws([&] { body.Validate(Sid{5},1); }));scene.base.PostPopVirtual(Sid{5},1);CHECK(Get(scene.raw,12,2)==0x5101 && scene.population.count==1 && scene.population.entries[0]==5 && scene.population.entries[1]==0xbbbb0000 && Get(scene.raw,36)==1 && scene.book.depth==0 && scene.raw[11]==1 && scene.raw[40]==8);
}
TEST_CASE(DudeParentCurrent_MissingBridgeNoticeKeepsQaBeforePopulation) { // 현재 Bridge 통지에 도달한 실패 앞에 QA만 남고 목록/프레임/부모를 실행하지 않는다.
    DudeCurrentScene scene;std::uint32_t bridge=74;RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks(),bridge);CHECK(Throws([&] { body.PostPop(Sid{5},1); }) && Get(scene.raw,12,2)==0x5101 && scene.population.count==0 && scene.population.entries[0]==0xaaaa0000 && scene.events.empty());
}
TEST_CASE(DudeParentCurrent_BridgeNoticeMutationUsesCurrentTypeAndNoIFrame) { // 통지 복귀 뒤 현재 슬롯/TYPE의 Run을 읽으며 없는 I의 -1도 실제 프레임 장치에 전달한다.
    DudeCurrentScene scene;std::uint32_t bridge=74;auto hooks=scene.Hooks();hooks.notifySurface=[&](Sid sid) { CHECK(sid==Sid{5} && Get(scene.raw,12,2)==0x5101);scene.events.push_back("N");scene.raw[10]=75;scene.raw[11]=1;scene.raw[40]=8;Put(scene.raw,0,0);scene.frames[75]=RiftTypeFrames({}); };RawDudePostPop body(scene.pool,scene.frames,scene.state,std::move(hooks),bridge);body.PostPop(Sid{5},1);CHECK(scene.events.size()==3 && scene.events[0]=="N" && scene.events[1]=="F-1" && scene.events[2]=="B" && scene.population.count==1 && Get(scene.raw,36)==0xffffffffU && scene.raw[11]==1);
}
TEST_CASE(DudeParentCurrent_BridgeComparisonUsesFullDword) { // BYTE TYPE74를 현재 DWORD330으로 줄여 비교하지 않는다.
    DudeCurrentScene scene;std::uint32_t bridge=74+256;RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks(),bridge);body.PostPop(Sid{5},1);CHECK(scene.population.count==1 && scene.events.size()==2 && scene.events[0]=="F1" && scene.book.depth==0);
}
TEST_CASE(DudeParentCurrent_FullAndSignedNegativeCapacitySkipStorage) { // 원문 signed jge에서 가득 찼거나 음수 용량이면 배열을 읽지 않고 프레임/부모를 계속한다.
    for (const auto input:{std::array<std::uint32_t,2>{1,1},std::array<std::uint32_t,2>{2,1},std::array<std::uint32_t,2>{0,0x80000000U}}) { // unsigned 비교와 전수 count 검사를 구별하는 세 현재 입력이다.
        DudeCurrentScene scene;scene.population.entries.clear();scene.population.BindCurrent({[]()->std::span<std::uint32_t> { throw std::logic_error("미선택 배열"); },[&] { return input[1]; },[&] { return input[0]; },[](auto) { throw std::logic_error("미선택 개수 쓰기"); }});RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks());body.PostPop(Sid{5},1);CHECK(scene.events.size()==2 && scene.events[0]=="F1" && scene.book.depth==0 && Get(scene.raw,12,2)==0x5101);
    }
}
TEST_CASE(DudeParentCurrent_CapturedIndexStoreThenCurrentCountIncrement) { // 배열 조회 효과 뒤 저장 index는 보존하고 inc는 그때의 현재 개수를 읽는다.
    DudeCurrentScene scene;scene.population.entries.clear();std::array<std::uint32_t,2> storage{0xaaaa0000,0xbbbb0000};std::uint32_t count=0;scene.population.BindCurrent({[&]()->std::span<std::uint32_t> { count=7;scene.raw[10]=75;return storage; },[] { return 2U; },[&] { return count; },[&](auto value) { CHECK(storage[0]==5);count=value; }});RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks());body.PostPop(Sid{5},1);CHECK(count==8 && storage[0]==5 && storage[1]==0xbbbb0000 && scene.events[0]=="F2" && Get(scene.raw,36)==2 && scene.book.depth==0);
}
TEST_CASE(DudeParentCurrent_NegativeIndexFailsOnlyAtSelectedStore) { // 음수 개수는 signed 비교를 통과한 뒤 실제 저장 위치에서 거부하며 QA를 되돌리지 않는다.
    DudeCurrentScene scene;scene.population.count=0xffffffffU;RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks());CHECK(Throws([&] { body.PostPop(Sid{5},1); }) && Get(scene.raw,12,2)==0x5101 && scene.population.count==0xffffffffU && scene.events.empty() && scene.population.entries[0]==0xaaaa0000);
}
TEST_CASE(DudeParentCurrent_ShortPhysicalArrayFailsAfterQa) { // 선택한 배열의 호스트 범위 오류는 최초 QA 뒤에만 확인한다.
    DudeCurrentScene scene;scene.population.entries.clear();scene.population.BindCurrent({[] { return std::span<std::uint32_t>{}; },[] { return 1U; },[] { return 0U; },[](auto) { throw std::logic_error("도달하지 않을 개수 쓰기"); }});RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks());CHECK(Throws([&] { body.PostPop(Sid{5},1); }) && Get(scene.raw,12,2)==0x5101 && scene.events.empty());
}
TEST_CASE(DudeParentCurrent_MissingFrameKeepsQaAndPopulation) { // 도달한 프레임 장치가 없으면 앞선 QA/목록 store와 count 증가를 보존한다.
    DudeCurrentScene scene;auto hooks=scene.Hooks();hooks.setFrame={};RawDudePostPop body(scene.pool,scene.frames,scene.state,std::move(hooks));CHECK(Throws([&] { body.PostPop(Sid{5},1); }) && Get(scene.raw,12,2)==0x5101 && scene.population.count==1 && scene.population.entries[0]==5 && scene.events.empty() && scene.book.depth==1);
}
TEST_CASE(DudeParentCurrent_CurrentTypeReadPrecedesMissingFrameDevice) { // TYPE+150의 물리 표 읽기가 프레임 장치 요구보다 앞선다.
    DudeCurrentScene scene;auto hooks=scene.Hooks();hooks.setFrame={};RawDudePostPop body(scene.pool,std::span(scene.frames).first(74),scene.state,std::move(hooks));bool physical=false;try { body.PostPop(Sid{5},1); }catch (const std::out_of_range&) { physical=true; }CHECK(physical && scene.population.count==1 && scene.population.entries[0]==5 && Get(scene.raw,12,2)==0x5101 && scene.events.empty());
}
TEST_CASE(DudeParentCurrent_MissingBaseKeepsFrameAndRegistration) { // 공통 부모 연결 실패 앞에 실제 QA/등록/프레임 장치 효과가 남는다.
    DudeCurrentScene scene;auto hooks=scene.Hooks();hooks.base={};RawDudePostPop body(scene.pool,scene.frames,scene.state,std::move(hooks));CHECK(Throws([&] { body.PostPop(Sid{5},1); }) && scene.population.count==1 && Get(scene.raw,36)==1 && scene.events==std::vector<std::string>{"F1"} && scene.book.depth==1);
}
TEST_CASE(DudeParentCurrent_ExplicitValidateAndCdConstructorStaySeparate) { // 명시 사전 검사와 CD 구성 계약은 실제 PATCH 호출의 앞에 삽입하지 않는다.
    DudeCurrentScene scene;RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks());CHECK(Throws([&] { body.Validate(Sid{5},1); }) && Get(scene.raw,12,2)==0xbeef && scene.population.count==0);SidPool cd(OriginalEdition::Cd1072,24000,true);CHECK(Throws([&] { RawDudePostPop missing(cd,{},scene.state,{}); }));
}
TEST_CASE(DudeParentCurrent_FirstPhysicalSidFailurePrecedesBase) { // flags1의 QA 저장은 풀 밖 SID를 거부하고 그 뒤 부모를 실행하지 않는다.
    DudeCurrentScene scene;RawDudePostPop body(scene.pool,scene.frames,scene.state,scene.Hooks());CHECK(Throws([&] { body.PostPop(Sid{24000},1); }) && scene.events.empty() && scene.population.count==0);
}
TEST_CASE(DudeParentCurrent_SpawnCurrentSignedGateDoesNotReadStorageOrType) { // 생성 거부 관문은 실제 현재 signed 개수/용량만 읽고 TYPE/좌표/배열을 미리 검사하지 않는다.
    for (const auto input:{std::array<std::uint32_t,2>{0,0},std::array<std::uint32_t,2>{2,1},std::array<std::uint32_t,2>{0,0x80000000U}}) { // 가득 참/초과/음수 용량은 실제 Factory에 도달하지 않는다.
        DudeCurrentScene scene;scene.population.entries.clear();scene.state.dudeType=188;scene.population.BindCurrent({[]()->std::span<std::uint32_t> { throw std::logic_error("미선택 생성 배열"); },[&] { return input[1]; },[&] { return input[0]; },[](auto) { throw std::logic_error("미선택 생성 개수 쓰기"); }});RawDudeSpawn spawn(scene.pool,scene.factory,scene.owner,scene.state,{});const auto before=scene.pool.FreeCount();CHECK(spawn.Spawn(std::bit_cast<float>(0x7fc12345U),0,9999)==Sid{} && scene.pool.FreeCount()==before);
    }
}
TEST_CASE(DudeParentCurrent_SpawnReadsBlockedAfterCurrentCapacity) { // 현재 용량 조회 효과가 바꾼 차단 전역을 그 뒤에 읽는다.
    DudeCurrentScene scene;scene.population.entries.clear();scene.state.dudeType=188;scene.population.BindCurrent({[] { return std::span<std::uint32_t>{}; },[&] { scene.state.blocked=true;return 2U; },[] { return 0U; },[](auto) {}});RawDudeSpawn spawn(scene.pool,scene.factory,scene.owner,scene.state,{});CHECK(spawn.Spawn(1,2,1)==Sid{} && scene.state.blocked);
}
TEST_CASE(DudeParentCurrent_SpawnMissingPopKeepsGenericCreationOwnerAndFrame) { // 현재 TYPE74의 실제 Factory/Owner와 frame0 뒤에만 미연결 Pop이 실패하며 NaN을 선행 거부하지 않는다.
    DudeCurrentScene scene;scene.state.dudeType=74;RawDudeSpawn spawn(scene.pool,scene.factory,scene.owner,scene.state,{});CHECK(Throws([&] { spawn.Spawn(std::bit_cast<float>(0x7fc12345U),12.5F,8); }));const auto born=scene.pool.Slot(Sid{6});CHECK(born[10]==74 && !(born[11]&1U) && born[34]==8 && Get(born,36)==0 && scene.population.count==0);
}
TEST_CASE(DudeParentCurrent_SpawnNegativeCountReachesActualFactoryAndSelectedPop) { // 음수 개수는 생성 관문만 통과하며 배열 저장은 실제 Pop 하위에 맡긴다.
    DudeCurrentScene scene;scene.population.count=0xffffffffU;scene.state.dudeType=74;std::uint32_t calls=0;DudeSpawnHooks hooks;hooks.pop=[&](Sid sid,float x,float y,auto flags) { CHECK(sid==Sid{6} && std::bit_cast<std::uint32_t>(x)==0x7fc12345U && y==12.5F && flags==0 && scene.pool.Slot(sid)[34]==8 && Get(scene.pool.Slot(sid),36)==0);++calls; };RawDudeSpawn spawn(scene.pool,scene.factory,scene.owner,scene.state,std::move(hooks));CHECK(spawn.Spawn(std::bit_cast<float>(0x7fc12345U),12.5F,8)==Sid{6} && calls==1 && scene.population.count==0xffffffffU);
}
