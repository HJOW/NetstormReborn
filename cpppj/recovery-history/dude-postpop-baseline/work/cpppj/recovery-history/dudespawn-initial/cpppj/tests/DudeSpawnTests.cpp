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
    BaseFileSystem files; // 보호 디렉터리/TAFF를 읽는 기존 실제 자산 저장소다.
    netstorm::client::GameAssets game; // 실제 Dude TYPE/SHP/팔레트를 해석한 현재 자산이다.
    std::vector<RiftTypeRecord> types; // 원본 로딩 번호와 실제 생성자 주소의 타입 표다.
    std::vector<RiftTypeFrames> frames; // 실제 Dude I00/64를 포함하는 현재 Run 표다.
    std::vector<SquidDisplayShape> shapes; // 같은130물리프레임의 실제 크기/핫스폿이다.
    // 같은 보호 자산을 읽기 전용으로 연결하며 창/장치를 만들지 않는다.
    explicit DudeAssets(OriginalEdition edition):files(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")),game(Register(),edition),types(game.TypeTable().Types().begin(),game.TypeTable().Types().end()),frames(types.size(),RiftTypeFrames({})),shapes(netstorm::client::SquidRenderer::Shapes(game,edition)) {
        // 물리 블록의 자산 번호70부터 실제 프레임 Run을 채운다.
        for (const auto& asset:game.Types()) frames[70+asset.block]=asset.definition.FrameTable();
        CHECK(types[70].footX==0 && types[70].footY==0 && types[70].maxHitPoints==20 && frames[70].Run('I').first==64 && shapes[70].frameCount==65);
        // 원본 준비용 미등록 base74는 합성 빈 타입이며 새 Dude의 보호 자료와 구분한다.
        types[74]={};
    }
    // 초기화 순서상 GameAssets 전에 같은 archive를 실제 등록한다.
    const BaseFileSystem& Register() { files.RegisterArchive(files.Root()/"netstorm.tarc");return files; }
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
