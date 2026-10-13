// 실제부모/Territory의독립관찰을같은현재Builder/3204BYTE/청크/owner로대조한다.
#include "RawSceneSupport.h"
#include "o/RawFortWorldLoad.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본입력의고정열순서다. 기대후처리/섬수/owner는포함하지않는다.
enum FortWorldField : std::size_t { Kind,Alternate,Owner,Count,Exists,Profile,Position,End,Depth,Mutation,TemplatePresent };
// 원본기록의16진BYTE를직접복원한다.
std::vector<std::uint8_t> WorldHex(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    // 각BYTE두글자만읽고그의미를다시계산하지않는다.
    for (std::size_t offset=0;offset<hex.size();offset+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(offset,2),nullptr,16)));
    return bytes;
}
// 전체원본block을복원하며비활성기록/padding을생략하지않는다.
std::vector<std::uint8_t> WorldBlocks(const std::string& text) {
    std::vector<std::uint8_t> bytes;
    // 저장run의그대로인바이트/횟수를사용한다.
    for (const auto& run:Split(text,';')) {
        const auto fields=Split(run,':');const auto raw=WorldHex(fields.at(0));
        // 원본block을해석없이그횟수만반복한다.
        for (std::uint32_t count=0;count<Number(fields.at(1));++count) bytes.insert(bytes.end(),raw.begin(),raw.end());
    }
    return bytes;
}
// 실제기존Builder의16방향·P변형·번호0..10합성코드입력이다.
RiftTypeFrames WorldPieceFrames() {
    std::vector<FrameCode> codes;
    // 원본도같은native입력표를읽으며frame결과를미리만들지않는다.
    for (std::uint8_t side='A';side<='P';++side)
        // 각방향에서실제패턴의번호검색을허용한다.
        for (std::uint8_t number=0;number<11;++number) codes.push_back({side,'P',number,0});
    codes.push_back(codes.front());return RiftTypeFrames(std::move(codes));
}
// 현재소비자청크를원본X열우선12BYTE로직렬화한다.
std::vector<std::uint8_t> WorldChunks(const ChunkMap& chunks) {
    std::vector<std::uint8_t> bytes(256*12);
    // X열이바깥이며각열의Y16칸을저장한다.
    for (std::int32_t x=0;x<16;++x)
        // 각청크의이웃record/ordinal폭도그대로남긴다.
        for (std::int32_t y=0;y<16;++y) {
            const auto& entry=chunks.At(x,y);const auto offset=static_cast<std::size_t>(x*16+y)*12;
            bytes[offset]=entry.island;bytes[offset+1]=static_cast<std::uint8_t>(entry.side);std::copy(entry.record.begin(),entry.record.end(),bytes.begin()+static_cast<std::ptrdiff_t>(offset+2));Put(bytes,offset+8,std::bit_cast<std::uint32_t>(entry.ordinal));
        }
    return bytes;
}
// 단일원본입력의실제풀/현재3204BYTE/후처리경계를소유한다.
struct FortWorldReplayScene {
    SidPool pool; // 현재Builder/Runtime가읽는단일판본풀이다.
    SquidHash hash; // 같은Runtime의현재공간참조다.
    ChunkMap chunks; // 실제주소동일성이있는world지도다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> raw{}; // 원본count+100개32BYTE/padding기록이다.
    FortFinishOwnerState modes; // Add/Bounds와같은mode전역이다.
    SquidPostPopList residences; // 선택Builder는owner가상전파를요청하지않는다.
    std::uint32_t revision{},changed{},piece{107},owner{},depth{}; // 현재원본DWORD저장소다.
    std::int32_t noRegion{127}; // Generate가호출직전다시읽는현재지역없음전역이다.
    std::vector<RiftTypeFrames> frames; // 원본과같은현재puzzlePiece프레임입력이다.
    std::vector<std::uint32_t> input; // native의11개입력DWORD다.
    std::vector<std::uint8_t> data; // 현재pos앞prefix와실제120BYTE섹션이다.
    FortTerritoryStream stream; // 현재pos/end를각실제child가읽는다.
    std::array<FortTemplate,9> templates; // currentTemplate는다음stage마다현재owner칸을다시읽는다.
    std::array<bool,9> present{}; // 진단callback이현재Template pointer를복구하는명시입력이다.
    std::vector<std::string> events; // 원본외부경계의순서를기록한다.
    std::size_t assertions{}; // 보고뒤실제반환한진단수다.
    std::unique_ptr<RawIslandRuntime> islands; // 위기록을그대로빌리며복사목록을만들지않는다.
    // 원본입력만구성하고최종지도/owner/후처리의기대값은계산하지않는다.
    FortWorldReplayScene(OriginalEdition edition,const std::string& fields):pool(edition,kCapacity,false),frames(edition==OriginalEdition::Patch1078 ? 188U : 171U,RiftTypeFrames({})) {
        // 모든현재DWORD를원본열순서로읽는다.
        for (const auto& field:Split(fields,',')) input.push_back(Number(field));CHECK(input.size()==11);
        owner=input[Owner];depth=input[Depth];raw.fill(0xa5);Put(raw,0,input[Count]);Put(raw,8,input[Exists]);frames[piece]=WorldPieceFrames();
        // 초기원본청크의모든12BYTE를준비한다.
        for (std::int32_t x=0;x<16;++x)
            // 다음input에서이전순회결과를재사용하지않는다.
            for (std::int32_t y=0;y<16;++y) { auto& entry=chunks.At(x,y);entry.island=127;entry.side='.';entry.record.fill(0xa5);entry.ordinal=0x12345678; }
        data.resize(input[Position]+120);if (input[Profile]) std::copy_n(std::array<std::uint8_t,6>{3,1,0x11,7,0xaa,0xbb}.begin(),6,data.begin()+input[Position]);
        if (input[Profile]==2) std::copy_n(std::array<std::uint8_t,6>{3,1,0x55,31,0xcc,0xdd}.begin(),6,data.begin()+input[Position]+18);
        stream={data,input[Position],input[End]};present.fill(input[TemplatePresent]!=0);
        // 각Template의flag는관찰용identity이며월드효과를대신계산하지않는다.
        for (std::size_t index=0;index<templates.size();++index) templates[index].flag=static_cast<std::uint8_t>(index);
        islands=std::make_unique<RawIslandRuntime>(pool,hash,raw,modes,residences,revision,changed,IslandRuntimeHooks{
            [](Sid,std::uint32_t) { throw std::logic_error("선택Builder는Residence owner경계를요청하지않는다"); },
            [](IslandRuntimeAssertion) { throw std::logic_error("선택Builder의Runtime진단입력불일치"); }});
    }
    // 기존Builder는실제실행하고아직복원하지않은상위child는명시관찰경계로남긴다.
    FortWorldLoadHooks Hooks() {
        FortWorldLoadHooks hooks;hooks.templatePresent=[this](std::uint32_t slot) { return present.at(slot); };
        hooks.territory=[this](std::uint32_t)->FortTerritoryStream& {
            if (input[Mutation]==1) owner=2;if (input[Mutation]==8) noRegion=22;if (input[Mutation]==9) owner=5;return stream;
        }; // 1/8변경은native PlaceAll진입과같이다음owner/noRegion읽기보다앞이다. Builder자체는owner1을보존한다.
        hooks.currentTemplate=[this](std::uint32_t slot)->const FortTemplate& { return templates.at(slot); };
        hooks.assertion=[this](FortWorldLoadAssertion reason) { ++assertions;events.push_back(reason==FortWorldLoadAssertion::MissingTerritoryTemplate ? "A:T" : "A:E");if (reason==FortWorldLoadAssertion::MissingTerritoryTemplate) present.at(input[Owner])=true; };
        hooks.loadingDepthChanged=[this](std::uint32_t value) { CHECK(depth==value);events.push_back("L:"+std::to_string(value)); };
        hooks.builderAssertion=[](RawIslandBuilderAssertion) { throw std::logic_error("유효방향/프레임의선택Builder진단불일치"); };
        hooks.generate=[this](const RawIslandBuilderState& state,std::int32_t region) { CHECK(state.owner==1 && state.bounds.left==1 && state.bounds.right==15);events.push_back("G:"+std::to_string(region));if (input[Mutation]==2) owner=3;if (input[Mutation]==7) Put(raw,8,1); };
        hooks.objects=[this](const FortTemplate& fort,std::uint32_t slot) { CHECK(fort.flag==slot);events.push_back("O:"+std::to_string(slot));if (input[Mutation]==3) owner=4; };
        hooks.money=[this](const FortTemplate& fort,std::uint32_t slot) { CHECK(fort.flag==slot);events.push_back("M:"+std::to_string(slot));if (input[Mutation]==4) owner=5; };
        hooks.badges=[this](const FortTemplate& fort,std::uint32_t slot) { CHECK(fort.flag==slot);events.push_back("B:"+std::to_string(slot));if (input[Mutation]==5) owner=6; };
        hooks.technology=[this](const FortTemplate& fort,std::uint32_t slot) { CHECK(fort.flag==slot);events.push_back("T:"+std::to_string(slot));if (input[Mutation]==6) owner=7; };
        hooks.populateIsland=[this](const RawIslandBuilderState& state,std::int32_t region) { CHECK(state.owner==1 && region==0);events.push_back("I:0"); };
        hooks.rebuildGraph=[this](bool reset) { CHECK(!reset);events.push_back("R:0"); };return hooks;
    }
    // 실제기록한경계요청만원본순서로연결한다.
    std::string Events() const {
        std::string out;
        // 모든경계요청의순서와인자를보존한다.
        for (const auto& event:events) { if (!out.empty()) out+=';';out+=event; }return out.empty() ? "-" : out;
    }
};
}

// 원본534고유입력의전체청크/3204BYTE/현재owner/depth와stage순서를전수대조한다.
TEST_CASE(FortWorldLoad_ReplaysBothNativeParentAndTerritory) {
    const auto fixture=LoadFixture(NETSTORM_FORTWORLDLOAD_FIXTURE);std::size_t checked=0;
    // 각원본행은새저장소에서시작하며기대Builder효과는계산하지않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==12);FortWorldReplayScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row[1]);
        RawFortWorldLoad loader(scene.pool,scene.chunks,*scene.islands,scene.frames,scene.piece,scene.owner,scene.noRegion,scene.depth,scene.Hooks());std::array<std::uint8_t,120> output{};
        if (scene.input[Kind]) loader.ReadTerritory(scene.owner,output);else loader.Load(std::bit_cast<std::int32_t>(scene.input[Alternate]));
        const auto islandFields=Split(row[9],',');auto expectedIslands=WorldBlocks(islandFields.at(1));std::vector<std::uint8_t> actualRecords(scene.raw.begin()+4,scene.raw.end());
        const bool same=std::to_string(scene.stream.position)==row[2] && std::to_string(scene.stream.end)==row[3] && std::to_string(scene.depth)==row[4] && std::to_string(scene.owner)==row[5] && std::to_string(scene.noRegion)==row[6] &&
            (!scene.input[Kind] || Hex(output)==row[7]) && WorldChunks(scene.chunks)==WorldBlocks(row[8]) && std::to_string(scene.islands->Count())==islandFields.at(0) && actualRecords==expectedIslands && scene.Events()==row[10] && std::to_string(scene.assertions)==row[11];CHECK(same);
        if (!same) { std::fprintf(stderr,"FortWorld %s row%zu input%s actual%s expected%s\n",row[0].c_str(),checked,row[1].c_str(),scene.Events().c_str(),row[10].c_str());return; }++checked;
    }
    CHECK(checked==534);
}

// 동일풀/필수효과/물리120BYTE경계를보존하며없는child를성공처리하지않는다.
TEST_CASE(FortWorldLoad_RejectsForeignPoolMissingChildAndTruncatedTerritory) {
    FortWorldReplayScene scene(OriginalEdition::Patch1078,"0,0,1,0,0,1,0,120,0,0,1");auto hooks=scene.Hooks();hooks.populateIsland={};
    CHECK(Throws([&] { RawFortWorldLoad bad(scene.pool,scene.chunks,*scene.islands,scene.frames,scene.piece,scene.owner,scene.noRegion,scene.depth,hooks); }));
    SidPool foreign(OriginalEdition::Patch1078,kCapacity,false);CHECK(Throws([&] { RawFortWorldLoad bad(foreign,scene.chunks,*scene.islands,scene.frames,scene.piece,scene.owner,scene.noRegion,scene.depth,scene.Hooks()); }));
    RawFortWorldLoad loader(scene.pool,scene.chunks,*scene.islands,scene.frames,scene.piece,scene.owner,scene.noRegion,scene.depth,scene.Hooks());scene.stream.bytes=std::span(scene.data).first(119);std::array<std::uint8_t,120> output{};
    CHECK(Throws([&] { loader.ReadTerritory(1,output); }));CHECK(scene.stream.position==0);
}
