// 원본Place/PlaceAll의전체청크/3204BYTE와같은실제월드의후속성장소비자를검사한다.
#include "RawSceneSupport.h"
#include "o/RawIslandBuilder.h"
#include "o/RawTerrainGrowth.h"
#include "o/RawIslandQueries.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 두PE의puzzlePiece 타입과fixture의원본물리record 수다.
constexpr std::uint32_t kRawBuilderPiece=107;
constexpr std::size_t kRawBuilderChunks=256,kRawBuilderIslands=100;
// 16방향·번호0..10의합성프레임입력은native입력과같고출력frame을미리계산하지않는다.
RiftTypeFrames BuilderFrames(std::uint32_t profile) {
    std::vector<FrameCode> codes;
    // 빈profile2외에는모든방향의원본패턴variation번호를공급한다.
    for (std::uint8_t side='A';side<='P';++side)
        // 순서를바꾼profile1도실제FindNumber가프레임을찾게한다.
        for (std::uint8_t number=0;number<11;++number) if (profile!=2) codes.push_back({side,'P',number,0});
    if (profile==1) std::reverse(codes.begin(),codes.end());
    if (profile!=2) codes.push_back(codes.front());
    // 부분누락profile3은A방향만제거하여진단/실제쓰기의교차순서를관찰한다.
    if (profile==3) std::erase_if(codes,[](const FrameCode& code) { return code.side=='A'; });
    return RiftTypeFrames(std::move(codes));
}
// 원본fixture의연속hex바이트를그대로읽는다.
std::vector<std::uint8_t> BuilderHex(const std::string& text) {
    if (text.size()%2) throw std::logic_error("IslandBuilder hex 폭 오류");std::vector<std::uint8_t> bytes;
    // 저장한각BYTE를원본값으로복원한다.
    for (std::size_t offset=0;offset<text.size();offset+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(offset,2),nullptr,16)));
    return bytes;
}
// 원본같은길이block RLE를풀며존재/owner/bounds를계산해대신하지않는다.
std::vector<std::uint8_t> BuilderBlocks(const std::string& text,std::size_t width,std::size_t count) {
    std::vector<std::uint8_t> bytes;
    // 모든run은같은물리폭의원본block을반복한다.
    for (const auto& run:Split(text,';')) {
        const auto fields=Split(run,':');CHECK(fields.size()==2);const auto block=BuilderHex(fields[0]);CHECK(block.size()==width);
        const auto repeat=Number(fields[1]);if (repeat>(count*width-bytes.size())/width) throw std::logic_error("IslandBuilder RLE 초과");
        // padding과count밖record도전체크기까지원문으로복원한다.
        for (std::uint32_t index=0;index<repeat;++index) bytes.insert(bytes.end(),block.begin(),block.end());
    }
    if (bytes.size()!=count*width) throw std::logic_error("IslandBuilder 전체물리record 길이 오류");return bytes;
}
// 현재ChunkMap의물리256개X열우선record를원본12BYTE로직렬화한다.
std::vector<std::uint8_t> BuilderChunkBytes(const ChunkMap& chunks) {
    std::vector<std::uint8_t> bytes(kRawBuilderChunks*12);
    // 네이티브inline 배열의X열을먼저읽는다.
    for (std::int32_t x=0;x<16;++x)
        // 각X열의Y16칸을같은offset으로저장한다.
        for (std::int32_t y=0;y<16;++y) {
            const auto& entry=chunks.At(x,y);const auto offset=static_cast<std::size_t>(x*16+y)*12;
            bytes[offset]=entry.island;bytes[offset+1]=static_cast<std::uint8_t>(entry.side);
            std::copy(entry.record.begin(),entry.record.end(),bytes.begin()+static_cast<std::ptrdiff_t>(offset+2));
            Put(bytes,offset+8,std::bit_cast<std::uint32_t>(entry.ordinal));
        }
    return bytes;
}
// fixture의builder전역/현재저장소를준비하며종료출력은native에서만읽는다.
struct BuilderReplayScene {
    SidPool pool; // 실제Runtime과source가공유하는판본별풀이다.
    SquidHash hash; // Runtime.Bounds/Add의현재실제월드해시참조이며복사본이아니다.
    ChunkMap world,custom; // pointer同一조건의world와별도임시지도를각각소유한다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> records{}; // 실제100개32바이트/개수DWORD다.
    FortFinishOwnerState modes; // Runtime의원래owner/현재owner와같은모드전역이다.
    SquidPostPopList residences; // 현재목록이며Place/Add/Bounds에서읽지않는다.
    std::uint32_t revision{},changed{},piece{kRawBuilderPiece}; // 실제Runtime전역과현재piece타입이다.
    std::vector<RiftTypeFrames> frames; // native와같은유효/역순/빈프레임입력이다.
    std::vector<char> events; // 원본보고뒤계속진행한진단의순서다.
    std::uint32_t assertions{}; // 현재고유입력의진단횟수다.
    std::unique_ptr<RawIslandRuntime> islands; // 위3204BYTE를같은풀/지도에서읽는다.
    // 판본크기/필수실제Runtime경계를준비한다.
    explicit BuilderReplayScene(OriginalEdition edition):pool(edition,32768,false),frames(edition==OriginalEdition::Patch1078 ? 188U : 171U,RiftTypeFrames({})) {
        islands=std::make_unique<RawIslandRuntime>(pool,hash,records,modes,residences,revision,changed,
            IslandRuntimeHooks{[](Sid,std::uint32_t) { throw std::logic_error("Add/Bounds는Residence owner를요청하지않는다"); },
                [this](IslandRuntimeAssertion) { throw std::logic_error("Builder fixture의Runtime진단은원본분류와불일치한다"); }});
    }
    // 각행에서native와같은full raw/padding/기존count밖영역을초기화한다.
    void Prepare(std::uint32_t count,std::uint32_t profile) {
        records.fill(0xa5);Put(records,0,count);frames[piece]=BuilderFrames(profile);events.clear();assertions=0;
        // 네이티브초기chunk12BYTE의모든필드를current getter에준비한다.
        for (std::int32_t x=0;x<16;++x)
            // 같은world/custom은둘다처음부터현재명시입력으로준비한다.
            for (std::int32_t y=0;y<16;++y) {
                ChunkEntry entry;entry.island=127;entry.side='.';entry.record.fill(0xa5);entry.ordinal=0x12345678;
                world.At(x,y)=entry;custom.At(x,y)=entry;
            }
    }
    // 각원본진단의순서를native기록의한문자와같게관찰한다.
    void Diagnostic(RawIslandBuilderAssertion assertion) {
        ++assertions;
        switch (assertion) {
        case RawIslandBuilderAssertion::InvalidBounds:events.push_back('B');break;
        case RawIslandBuilderAssertion::EqualRange:events.push_back('E');break;
        case RawIslandBuilderAssertion::OddDirection:events.push_back('O');break;
        case RawIslandBuilderAssertion::MissingFrame:events.push_back('F');break;
        case RawIslandBuilderAssertion::ChunkRange:events.push_back('R');break;
        case RawIslandBuilderAssertion::BoundsMissingIsland:events.push_back('M');break;
        case RawIslandBuilderAssertion::BoundsNotFound:events.push_back('N');break;
        }
    }
    // 진단이없으면-이며중복진단도발생순서로유지한다.
    std::string Events() const {
        std::string text;
        // 모든원본진단요청을세미콜론으로구분한다.
        for (const auto event:events) { if (!text.empty()) text+=';';text+=event; }return text.empty() ? "-" : text;
    }
};
}

// Place/PlaceAll의전수원본출력을모든기존record/padding·청크·진단순서까지대조한다.
TEST_CASE(IslandBuilder_ReplaysBothNativeFullStorage) {
    const auto fixture=LoadFixture(NETSTORM_ISLANDBUILDER_FIXTURE);std::size_t checked=0;
    // 양판본고유row만재생하며두x87관찰은여기서고유검사로중복계수하지않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==10);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto input=Split(row[2],',');CHECK(input.size()==14);std::array<std::uint32_t,14> value{};
        // 원본DWORD입력은부호변환전에그대로저장한다.
        for (std::size_t field=0;field<value.size();++field) value[field]=Number(input[field]);
        CHECK(value[10]==16 && value[11]==16);BuilderReplayScene scene(edition);scene.Prepare(value[3],value[9]);
        RawIslandBuilderState state{{std::bit_cast<std::int32_t>(value[4]),std::bit_cast<std::int32_t>(value[5]),
            std::bit_cast<std::int32_t>(value[6]),std::bit_cast<std::int32_t>(value[7])},value[1]};
        auto& target=value[0] ? scene.world : scene.custom;RawIslandBuilder builder(target,scene.world,*scene.islands,scene.frames,scene.piece,state,
            [&scene](RawIslandBuilderAssertion assertion) { scene.Diagnostic(assertion); });
        const auto record=BuilderHex(row[3]);std::uint32_t result=0;
        if (row[1]=="Place") { CHECK(record.size()==6);result=builder.Place(std::span<const std::uint8_t,6>(record.data(),6),value[2],value[8]!=0); }
        else builder.PlaceAll(record,std::bit_cast<std::int32_t>(value[12]),std::bit_cast<std::int32_t>(value[13]));
        const auto chunks=BuilderBlocks(row[5],12,kRawBuilderChunks);const auto split=row[7].find(',');CHECK(split!=std::string::npos);
        auto islands=BuilderBlocks(row[7].substr(split+1),32,kRawBuilderIslands);islands.insert(islands.begin(),4,0);Put(islands,0,Number(row[7].substr(0,split)));
        const bool same=result==Number(row[4]) && BuilderChunkBytes(target)==chunks &&
            std::equal(islands.begin(),islands.end(),scene.records.begin()) && scene.Events()==row[8] && scene.assertions==Number(row[9]);
        CHECK(same);
        if (!same) { std::fprintf(stderr,"IslandBuilder %s %s row=%zu input=%s result=%u/%s events=%s/%s\n",row[0].c_str(),row[1].c_str(),checked,row[2].c_str(),result,row[4].c_str(),scene.Events().c_str(),row[8].c_str());break; }
        if (!value[0]) CHECK(BuilderChunkBytes(scene.world)==BuilderBlocks(row[6],12,kRawBuilderChunks));
        CHECK(builder.InWorld()==(value[0]!=0) && &builder.Pool()==&scene.pool);++checked;
    }
    CHECK(checked==fixture.rows.size());
}

// 실제World의한Pool/Runtime/ChunkMap을후속Count/owner/Matches와원본정상성장이직접소비한다.
TEST_CASE(IslandBuilder_LiveWorldChunkRuntimeAndGrowthConsumer) {
    // 원래ownerBYTE/현재ownerDWORD의분리와같은현재청크의재조회는양판본에적용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));frames[kRawBuilderPiece]=BuilderFrames(0);
        std::vector<SquidDisplayShape> shapes(types.size());BridgeConnectState bridge;
        bridge.bridgeType=82;bridge.noIslandType=157;bridge.islandType=94;bridge.stalagType=95;bridge.connectorType=155;
        netstorm::client::RawSurfaceWorld world(edition,types,frames,shapes,bridge,97);const auto core=world.Core();
        std::array<std::uint8_t,RawIslandRuntime::kBytes> records{};FortFinishOwnerState modes;SquidPostPopList residences;
        std::uint32_t revision=0,changed=0,piece=kRawBuilderPiece,diagnostics=0;ChunkMap chunks;RawIslandBuilderState state;state.owner=2;
        RawIslandRuntime islands(core.pool,core.hash,records,modes,residences,revision,changed,
            IslandRuntimeHooks{[&owner=core.owner](Sid sid,std::uint32_t player) { owner.Set(sid,player); },
                [&diagnostics](IslandRuntimeAssertion) { ++diagnostics; }});islands.Reset();
        RawIslandBuilder builder(chunks,chunks,islands,core.frames,piece,state,[&diagnostics](RawIslandBuilderAssertion) { ++diagnostics; });
        const std::array<std::uint8_t,6> record{3,1,0,7,0xaa,0xbb};const auto region=builder.Place(record,0);
        CHECK(region==0 && islands.Count()==1 && islands.Exists(0)==1 && islands.Owner(0)==2 && islands.Matches(0,2,0));
        const auto bounds=chunks.Bounds(0);const auto saved=islands.Bounds(0);
        CHECK(saved.left==bounds.left*16 && saved.top==bounds.top*16 && saved.right==bounds.right*16 && saved.bottom==bounds.bottom*16);
        islands.Owner(0,3);RawIslandQueries queries(islands,modes);
        CHECK(islands.Owner(0)==3 && islands.Matches(0,2,0) && queries.CountOwned(3)==1 && revision==1);
        TerrainContextGlobals globals;RawTerrainContext context(edition,2,0,DefaultChunkRect(),globals,
            [&diagnostics](TerrainContextAssertion) { ++diagnostics; });RawTerrainGrowth growth(context,chunks,globals);growth.RunNormal();
        CHECK(std::count(context.Codes().begin(),context.Codes().end(),static_cast<std::uint8_t>('x'))>0);
        CHECK(context.Map().codes.data()==context.Codes().data() && context.Map().regions.data()==context.Regions().data());
        // 지역BYTE는실제builder가등록한현재섬번호며후속소유자변경으로대신하지않는다.
        for (std::size_t cell=0;cell<context.Codes().size();++cell) if (context.Codes()[cell]=='x') CHECK(context.Regions()[cell]==0);
        CHECK(islands.Count()==1 && builder.InWorld() && &builder.Pool()==&core.pool && diagnostics==0);
    }
}

// 형식/필수진단/물리Terr범위를효과전거부하며정상empty범위는원본진단후반환한다.
TEST_CASE(IslandBuilder_RejectsMissingPhysicalStorageAndEffects) {
    BuilderReplayScene scene(OriginalEdition::Patch1078);scene.Prepare(2,0);RawIslandBuilderState state;
    CHECK(Throws([&] { RawIslandBuilder invalid(scene.world,scene.world,*scene.islands,scene.frames,scene.piece,state,{}); }));
    RawIslandBuilder builder(scene.world,scene.world,*scene.islands,scene.frames,scene.piece,state,
        [&scene](RawIslandBuilderAssertion assertion) { scene.Diagnostic(assertion); });const auto before=scene.records;
    std::array<std::uint8_t,120> records{};CHECK(Throws([&] { builder.PlaceAll(std::span(records).first(119)); }));
    CHECK(Throws([&] { builder.PlaceAll(records,-1,20); }));CHECK(Throws([&] { builder.PlaceAll(records,0,21); }));
    builder.PlaceAll(records,4,4);CHECK(scene.Events()=="E" && scene.records==before);
}
