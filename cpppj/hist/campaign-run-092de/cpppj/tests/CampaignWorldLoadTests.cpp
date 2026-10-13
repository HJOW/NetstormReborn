// actual미션부모의하위요청경계/현재두owner/전체Template·Player·Chunk·Island를독립원본과대조한다.
#include "DudeSpawnSupport.h"
#include "o/RawCampaignWorldLoad.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
using namespace netstorm::test::dudespawn;
namespace {
// 경계관찰범위밖의하위효과는기본성공으로완료처리하지않는다.
[[noreturn]] void MissingCampaignChild() { throw std::logic_error("actual캠페인부모관찰범위밖하위효과"); }
// 원본block run 전체를padding까지복원한다.
std::vector<std::uint8_t> CampaignBlocks(const std::string& text) {
    std::vector<std::uint8_t> bytes;
    // 각관찰run의실제BYTE와반복수만사용한다.
    for (const auto& item:Split(text,';')) { const auto parts=Split(item,':');std::vector<std::uint8_t> block;
        // 낮은정렬을요구하지않고두자리HEX를그대로읽는다.
        for (std::size_t index=0;index<parts[0].size();index+=2) block.push_back(static_cast<std::uint8_t>(std::stoul(parts[0].substr(index,2),nullptr,16)));
        // 동일block을원래물리순서그대로남긴다.
        for (std::uint32_t count=0;count<Number(parts[1]);++count) bytes.insert(bytes.end(),block.begin(),block.end());
    }
    return bytes;
}
// 원본과같은176개P변형및중복첫코드의명시입력이며선택결과를미리만들지않는다.
RiftTypeFrames CampaignFrames() {
    std::vector<FrameCode> codes;
    // 실제현재puzzlePiece의16방향입력이다.
    for (std::uint8_t side='A';side<='P';++side)
        // 각방향의0..10 번호검색을같은기존decoder에맡긴다.
        for (std::uint8_t number=0;number<11;++number) codes.push_back({side,'P',number,0});codes.push_back(codes.front());return RiftTypeFrames(std::move(codes));
}
// 현재청크의모든BYTE를원본X열우선순서로직렬화한다.
std::vector<std::uint8_t> CampaignChunks(const ChunkMap& chunks) {
    std::vector<std::uint8_t> bytes(256*12);
    // X열안의Y16기록을그대로남긴다.
    for (std::int32_t x=0;x<16;++x) for (std::int32_t y=0;y<16;++y) { const auto& entry=chunks.At(x,y);const auto offset=static_cast<std::size_t>(x*16+y)*12;bytes[offset]=entry.island;bytes[offset+1]=static_cast<std::uint8_t>(entry.side);std::copy(entry.record.begin(),entry.record.end(),bytes.begin()+offset+2);Put(bytes,offset+8,std::bit_cast<std::uint32_t>(entry.ordinal)); }
    return bytes;
}
// 외부요청은전수비교하며purector/attach/PlaceAll/선택Finish의내부진입계수는별도Native에보존한다.
std::string CampaignExternalEvents(const std::string& native) {
    std::string result;
    // CPP공개callback으로관찰되는원본요청만동일순서로선택한다.
    for (const auto& event:Split(native,';')) { if (event=="C" || event=="A" || event=="P:0:20" || event=="F:1") continue;if (!result.empty()) result+=';';result+=event; }return result;
}
// lowerPlayer/Generate/Object/Graph/UI는명시관찰경계이며실제Builder/Territory만같은raw저장소를쓴다.
struct CampaignReplayScene {
    std::vector<std::uint32_t> input; // owner/battle/editor/profile/pos/end/depth/mutation/subscriber/handle 입력이다.
    SidPool pool; // 같은Factory/Player/Runtime판본을소유한다.
    const DudeAssets& assets; // 기존152Recipe와보호WarBeginsTerritory120을읽기재사용한다.
    SquidHash hash; // Runtime과같은현재해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 같은Factory의현재공간참조다.
    SquidUnpop unpop; // 선택Ctor의기존공간계약이다.
    SquidPostPopState book; // lowerPop은명시경계라가짜공통장부를갱신하지않는다.
    SquidPostPop post; // 실제Player모듈의VT/풀계약이다.
    SquidFactory factory; // lowerPlayerwrapper의152Recipe는실제재사용한다.
    SquidOwnerMode mode; // 이직접Playerwrapper관찰의현재모드다.
    SquidOwner owner; // 실제공통Owner를사용한다.
    std::vector<std::uint8_t> players; // native의전체9Player이름/stale BYTE를그대로공급한다.
    PlayerWorldState playerState; // 같은현재Template/battle두owner다.
    std::unique_ptr<RawPlayerWorld> player; // lowerPop경계와실제wrapper를분리한다.
    ChunkMap chunks; // 같은주소의실제지도다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> islandBytes; // 현재count+100raw32기록이다.
    FortFinishOwnerState modes; // 기존Runtime Add/Bounds가읽는다.
    SquidPostPopList residences; // 선택Builder는가상Residence효과를요청하지않는다.
    std::uint32_t revision{},changed{},piece{107},depth{}; // 실제현재RawBuilder/Template전역이다.
    std::int32_t noRegion{127}; // Generate직전의현재지역없음이다.
    std::vector<RiftTypeFrames> frames; // native와동일puzzlePiece선택입력이다.
    std::unique_ptr<RawIslandRuntime> islands; // 위3204BYTE를같은풀에바인딩한다.
    std::vector<std::uint8_t> data; // streamprefix+실제120BYTE 입력이다.
    FortTerritoryStream stream; // 같은현재pos/end를getter가직접쓴다.
    std::array<FortTemplate,9> templates; // 다음Objects직전선택되는현재Template identity다.
    std::array<std::uint8_t,180> headers; // 실제parent가+8 DWORD만쓰는Template9 header다.
    std::array<std::uint8_t,64> handle; // 현재mySubHandle의stale BYTE를보존한다.
    std::vector<std::uint8_t> translated; // 실제NUL포함번역결과의명시입력이다.
    CampaignWorldGlobals globals; // subscriber/editor와선택missionPost1이다.
    std::vector<std::string> events; // native하위요청/진단의순서다.
    std::uint32_t assertions{}; // 현재stream진단수다.
    // 기대출력없이상위/하위경계입력과원본전체저장소를준비한다.
    explicit CampaignReplayScene(const std::vector<std::string>& row):pool(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,true),assets(Assets(pool.Edition())),unpop(pool,hash,spots),post(pool,assets.types,book),factory(pool,assets.types,false,&unpop),owner(pool,assets.types,book,mode),players(9*(pool.Edition()==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)),frames(assets.types.size(),RiftTypeFrames({})) {
        // native의원래순서대로입력DWORD를읽는다.
        for (const auto& field:Split(row[1],',')) input.push_back(Number(field));CHECK(input.size()==10);playerState={1,input[1],input[0],152,false,false};globals={0xabcdef01,input[8],input[2],true};depth=input[6];islandBytes.fill(0xa5);Put(islandBytes,0,0);headers.fill(0xa5);handle.fill(0x7e);std::fill(players.begin(),players.end(),std::uint8_t{0xa5});frames[piece]=CampaignFrames();
        // 실제Template managerpointer비트와관찰용identity를원래9칸에입력한다.
        for (std::size_t slot=0;slot<9;++slot) { Put(headers,slot*20,0x11a00100);templates[slot].flag=static_cast<std::uint8_t>(slot); }
        // 같은지도전체의stale record/ordinal을원본과동일하게준비한다.
        for (std::int32_t x=0;x<16;++x) for (std::int32_t y=0;y<16;++y) { auto& entry=chunks.At(x,y);entry.island=127;entry.side='.';entry.record.fill(0xa5);entry.ordinal=0x12345678; }
        data.resize(input[4]+120);if (input[3]==3) { const auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());CHECK(fort.territory.size()==120);std::copy(fort.territory.begin(),fort.territory.end(),data.begin()+input[4]); }
        else { if (input[3]) std::copy_n(std::array<std::uint8_t,6>{3,1,0x11,7,0xaa,0xbb}.begin(),6,data.begin()+input[4]);if (input[3]==2) std::copy_n(std::array<std::uint8_t,6>{3,1,0x55,31,0xcc,0xdd}.begin(),6,data.begin()+input[4]+18); }
        stream={data,input[4],input[5]};const std::string name=input[9]==0 ? "" : input[9]==1 ? "You" : "new-handle";translated.assign(name.begin(),name.end());translated.push_back(0);
        islands=std::make_unique<RawIslandRuntime>(pool,hash,islandBytes,modes,residences,revision,changed,IslandRuntimeHooks{[](Sid,std::uint32_t) { MissingCampaignChild(); },[](IslandRuntimeAssertion) { MissingCampaignChild(); }});
        PlayerWorldHooks hooks;hooks.assertion=[](PlayerWorldAssertion) { MissingCampaignChild(); };hooks.find=[](float,float,std::uint32_t) { return Sid{}; }; // native의lowerPlayer입구는기존후보를생성하지않는명시관찰경계다.
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };hooks.setOwner=[this](Sid sid,std::uint32_t value) { owner.Set(sid,value); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { CHECK(pool.Slot(sid)[10]==152 && x==static_cast<float>(playerState.battleOwner+10) && y==1.0f && flags==0);events.push_back("L:"+std::to_string(playerState.battleOwner));if (input[7]==3) playerState.battleOwner=3;if (input[7]==4) globals.editor=0;if (input[7]==5) playerState.templateOwner=5; }; // actual Pop/최초Regular는별도14전수이며이두부모검사는하위요청경계다.
        hooks.reserveRegular=[]()->bool { MissingCampaignChild(); };hooks.addRegular=[](Sid,std::uint32_t,float) { MissingCampaignChild(); };player=std::make_unique<RawPlayerWorld>(pool,post,players,playerState,std::move(hooks));
    }
    // 기존실제120getter만재사용하며generic부모의다른child를실행하지않는다.
    FortWorldLoadHooks TerritoryHooks() {
        FortWorldLoadHooks hooks;hooks.templatePresent=[](std::uint32_t) { return true; };hooks.territory=[this](std::uint32_t)->FortTerritoryStream& { if (input[7]==6) playerState.templateOwner=6;return stream; };
        hooks.currentTemplate=[this](std::uint32_t slot)->const FortTemplate& { return templates.at(slot); };hooks.assertion=[this](FortWorldLoadAssertion) { events.push_back("A:E");++assertions; };hooks.loadingDepthChanged=[this](std::uint32_t value) { CHECK(depth==value);events.push_back("L:"+std::to_string(value)); };hooks.builderAssertion=[](RawIslandBuilderAssertion) { MissingCampaignChild(); };
        hooks.generate=[](const RawIslandBuilderState&,std::int32_t) { MissingCampaignChild(); };hooks.objects=hooks.money=hooks.badges=hooks.technology=[](const FortTemplate&,std::uint32_t) { MissingCampaignChild(); };hooks.populateIsland=[](const RawIslandBuilderState&,std::int32_t) { MissingCampaignChild(); };hooks.rebuildGraph=[](bool) { MissingCampaignChild(); };return hooks;
    }
    // actualparent의상위body만대조하며하위전체완료로확대하지않는다.
    std::vector<std::string> Run() {
        RawFortWorldLoad territory(pool,chunks,*islands,frames,piece,playerState.templateOwner,noRegion,depth,TerritoryHooks());CampaignWorldLoadHooks hooks;
        hooks.translateYou=[this] { events.push_back("Y");return std::span<const std::uint8_t>(translated); };hooks.setConfig=[this](std::string_view key,std::span<const std::uint8_t> bytes) { CHECK(key=="fort.mySubHandle" && bytes.data()==handle.data());events.push_back("S");if (input[7]==1) playerState.templateOwner=2; };hooks.refreshFortSpecification=[this] { events.push_back("V");if (input[7]==2) playerState.battleOwner=2; };hooks.builderAssertion=[](RawIslandBuilderAssertion) { MissingCampaignChild(); };
        hooks.generate=[this](const RawIslandBuilderState& current,std::int32_t region) { CHECK(current.owner==1 && current.bounds.left==1 && current.bounds.top==1 && current.bounds.right==15 && current.bounds.bottom==15);events.push_back("G:"+std::to_string(region));if (input[7]==8) playerState.templateOwner=8; };hooks.currentTemplate=[this](std::uint32_t slot)->const FortTemplate& { return templates.at(slot); };hooks.objects=[this](const FortTemplate& current,std::uint32_t slot) { CHECK(current.flag==slot);events.push_back("O:"+std::to_string(slot));if (input[7]==9) playerState.battleOwner=8; };hooks.rebuildGraph=[this](bool reset) { CHECK(!reset);events.push_back("R:0"); };hooks.resetCamera=[this] { events.push_back("K");if (input[7]==7) playerState.battleOwner=7;if (input[7]==10) std::copy_n("camera-name",12,handle.begin()); };
        RawCampaignWorldLoad parent(chunks,*islands,frames,piece,noRegion,territory,*player,globals,headers,handle,std::move(hooks));parent.Load();std::string actions;
        // 현재콜백을통해관찰한모든하위요청을순서대로직렬화한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        return {std::to_string(globals.mySubscriber),Hex(headers),Hex(handle),Hex(players),std::to_string(playerState.templateOwner),std::to_string(playerState.battleOwner),std::to_string(globals.editor),std::to_string(stream.position),std::to_string(stream.end),std::to_string(depth),Hex(CampaignChunks(chunks)),Hex(islandBytes),actions,std::to_string(assertions)};
    }
};
// 같은판본의원본65행을같은입력에서재생하고상위전체BYTE/하위요청을대조한다.
void ReplayCampaign(std::string_view edition) {
    const auto fixture=LoadFixture(NETSTORM_CAMPAIGN_WORLDLOAD_FIXTURE);std::size_t count=0;int failures=0;
    // 원본기대출력은현재body의실행뒤에만읽는다.
    for (const auto& row:fixture.rows) { if (row[0]!=edition) continue;CHECK(row.size()==16);CampaignReplayScene scene(row);const auto output=scene.Run();auto expected=std::vector<std::string>(row.begin()+2,row.end());expected[10]=Hex(CampaignBlocks(expected[10]));const auto island=Split(expected[11],',');auto bytes=CampaignBlocks(island.at(1));std::vector<std::uint8_t> full(4);Put(full,0,Number(island.at(0)));full.insert(full.end(),bytes.begin(),bytes.end());expected[11]=Hex(full);expected[12]=CampaignExternalEvents(expected[12]);CHECK(output==expected);
        if (output!=expected) { for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("ActualCampaign %s row%zu col%zu expected%s actual%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());if (++failures>=3) break; }++count;
    }
    CHECK(failures==0 && count==65);
}
}
// 패치실제부모65입력의전체현재BYTE/두owner/하위순서를대조한다.
TEST_CASE(CampaignWorldLoad_PatchNativeParentRequestsAndFullCurrentBytes) { ReplayCampaign("originals"); }
// CD의현재Template선택/즉시depth복구와같은섬자료65입력을대조한다.
TEST_CASE(CampaignWorldLoad_CdNativeParentRequestsAndFullCurrentBytes) { ReplayCampaign("originalCD"); }
