// 독립원본118회의현재지도/난수/가상순서/직접frame·HP쓰기를같은기존배치몸체로비교한다.
#include "RawSceneSupport.h"
#include "o/RawIslandPopulation.h"
#include "o/RawIslandAdd.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // fixture 입력과 실제 하위 장치의 관측만 준비한다.
std::vector<std::string> PopulationFields(std::string_view text) { std::vector<std::string> result;std::size_t begin{};for (;;) { const auto end=text.find('\t',begin);result.emplace_back(text.substr(begin,end-begin));if (end==text.npos) return result;begin=end+1; } } //탭필드의정확한문자열을보존한다.
std::string PopulationHex(std::span<const std::uint8_t> bytes) { constexpr char digits[]="0123456789abcdef";std::string result;for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; } //원문전체지도BYTE를비교한다.
std::string PopulationJoin(const std::vector<std::string>& events) { std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result.empty()?"-":result; } //원문호출순서에정렬이나중복제거를하지않는다.
template<class... Values> void PopulationEvent(std::vector<std::string>& events,std::string name,Values... values) { ((name+=':'+std::to_string(values)),...);events.push_back(std::move(name)); } //Native와같은DWORD비트인자를기록한다.
FortRandomRectangle PopulationRectangle(std::uint32_t profile) { return std::array<FortRandomRectangle,5>{{{16,32,64,96},{15.25f,21.5f,16.1f,23.9f},{-15.25f,-3.5f,4.5f,9.5f},{20,21,20,21},{20,21,19,19}}}.at(profile); } //명시입력사각형이며기대결과를계산하지않는다.
std::vector<std::uint8_t> PopulationMap(const ChunkMap& map) {
    std::vector<std::uint8_t> bytes(3084);Put(bytes,0,map.Mode());Put(bytes,4,map.XLength());Put(bytes,8,map.YLength());
    for (std::size_t index=0;index<256;++index) { const auto& entry=map.Entries()[index];const auto at=12+index*12;bytes[at]=entry.island;bytes[at+1]=static_cast<std::uint8_t>(entry.side);std::copy(entry.record.begin(),entry.record.end(),bytes.begin()+at+2);Put(bytes,at+8,static_cast<std::uint32_t>(entry.ordinal)); } //실제256행/헤더를원문배치로직렬화한다.
    return bytes;
}
struct PopulationScene {
    SidPool pool{OriginalEdition::Patch1078,24000};SquidHash hash;ChunkMap map; // 원문 서버/예측 free-list 영역을 모두 확보하는 유효 용량과 같은 현재 지도다.
    std::vector<RiftTypeRecord> types{188};SquidPostPopList residences;FortFinishOwnerState modes; //기존40ded0몸체의현재참조다.
    std::array<std::uint8_t,3204> islandBytes{};std::uint32_t redraw{},revision{},calls{},seed{},fort{},primary{},fallback{},residence{89},replacement{158};bool boss{},fortMode{}; //입력전역/기존Runtime물리와관측카운터다.
    std::vector<std::string> events;std::unique_ptr<RawIslandRuntime> islands;std::unique_ptr<RawFortFinishOwner> placement;std::unique_ptr<RawIslandPopulation> population; //참조보다먼저반납하는실제복원몸체다.
    explicit PopulationScene(const std::vector<std::string>& row) {
        const auto number=[&](std::size_t at) { return static_cast<std::uint32_t>(std::stoull(row.at(at))); };const auto at=number(9),flip=number(5),hp=number(8),mutate=number(10);seed=number(2);fort=number(3);fortMode=fort!=0;boss=number(4)!=0;primary=number(6);fallback=number(7);
        CHECK(pool.Allocate()==Sid{15000});CHECK(pool.Allocate()==Sid{15001}); // 실제 서버 첫15000을 거쳐 Native 장치가 선택한15001을 확보한다. 다음 생성은15002다.
        for (auto& entry:map.Entries()) { entry.island=127;entry.side='.';entry.record.fill(0xa5);entry.ordinal=0x12345678; } //전체실제지도와미사용영역의명시입력이다.
        for (const auto point:std::array<std::array<int,2>,4>{{{1,2},{2,2},{1,3},{2,3}}}) { auto& entry=map.Entries()[point[1]*16+point[0]];entry.island=3;entry.record={3,1,0x21,static_cast<std::uint8_t>(seed),0xab,0xcd}; } //Native 도구가 준비한 물리offset을 그대로 쓴다. 실제 At은x*16+y이므로 이 입력을 임의 전치하지 않는다.
        Put(islandBytes,0,4);Put(islandBytes,4+3*32+4,1);Put(islandBytes,4+3*32+8,3);
        islands=std::make_unique<RawIslandRuntime>(pool,hash,islandBytes,modes,residences,redraw,revision,IslandRuntimeHooks{[](auto,auto) { throw std::logic_error("미선택Runtime거주구역owner"); },[](auto) { throw std::logic_error("미선택Runtime진단"); }});
        FortFinishOwnerHooks random;random.regionBounds=[](auto)->FortRandomRectangle { throw std::logic_error("직접40ded0의미선택bounds"); };random.wallSeconds=[]()->double { throw std::logic_error("직접40ded0의미선택시각"); };random.invalidType=[this] { PopulationEvent(events,"D",29); };
        random.place=[this,at](GameRandom& local,const FortRandomPlaceRequest& request) { ++calls;PopulationEvent(events,"P",request.type,std::bit_cast<std::uint32_t>(request.x),std::bit_cast<std::uint32_t>(request.y),static_cast<std::uint32_t>(request.region),request.mode,request.force,local.State());return calls>=at ? 15001U : 0U; }; //기존40ded0/PRNG는실제로실행하고개별World배치경계만관측한다.
        placement=std::make_unique<RawFortFinishOwner>(pool,types,residences,modes,std::move(random));IslandPopulationHooks hooks;
        hooks.assertion=[this,flip](auto assertion) { constexpr std::array<std::uint32_t,5> lines{0xc2,0x139,0xc3,0xd1,0x118};const auto line=lines.at(static_cast<std::size_t>(assertion));PopulationEvent(events,"A",line);if (flip && (line==0xc3 || line==0xd1)) boss=true; };
        hooks.wallSeconds=[this] { const auto words=std::bit_cast<std::array<std::uint32_t,2>>(2.5);PopulationEvent(events,"T",words[0],words[1]);return 2.5; };
        hooks.position=[this](Sid sid) { PopulationEvent(events,"position",sid.value);return std::array<float,2>{55.25f,66.5f}; };
        hooks.destroy=[this,mutate](Sid sid,auto flags) { PopulationEvent(events,"destroy",sid.value,flags);if (mutate) map.Entries()[2*16+1].record[0]=0xff; }; //Native의+12+(2*16+1)*12+2와 같은 물리BYTE만 바꾼다.
        hooks.create=[this](auto type,auto flags) { PopulationEvent(events,"create",type,flags);const auto sid=pool.Allocate();CHECK(sid==Sid{15002});auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x10012000U);raw[10]=static_cast<std::uint8_t>(type);raw[11]=4;return sid; };
        hooks.defaultFrame=[this](auto type) { PopulationEvent(events,"type",type);return -7; };hooks.hasHitPoints=[this,hp](auto) { PopulationEvent(events,"hasHp");return hp!=0; };hooks.typeHitPoints=[this](auto type) { PopulationEvent(events,"type",type);PopulationEvent(events,"maxHp");return 87; };
        hooks.setOwner=[this](auto,auto owner) { PopulationEvent(events,"owner",3);PopulationEvent(events,"setOwner",owner); };hooks.pop=[this](auto,float x,float y,auto flags) { PopulationEvent(events,"pop",std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags); };hooks.refreshProduction=[this] { PopulationEvent(events,"refresh"); };
        population=std::make_unique<RawIslandPopulation>(pool,map,map,*islands,*placement,IslandPopulationGlobals{boss,fortMode,residence,primary,fallback,replacement},std::move(hooks));
    } //입력의현재전역은참조이며부모호출동안같은주소를유지한다.
};
}

TEST_CASE(IslandPopulation_ReplaysNative118ActualMapRandomAndEffects) {
    std::ifstream input(NETSTORM_ISLAND_POPULATION_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) {
        if (line.empty()) continue;const auto row=PopulationFields(line);CHECK(row.size()==18);if (row.size()!=18) continue;PopulationScene scene(row);
        if (row[0]=="Residence") scene.population->Residence(3);else if (row[0]=="Populate") scene.population->Populate(3);else scene.population->ReplaceInitial(PopulationRectangle(static_cast<std::uint32_t>(std::stoul(row[1]))),3);
        const auto trace=PopulationJoin(scene.events);if (trace!=row[12]) std::fprintf(stderr,"Population행%zu %s profile%s expected%s actual%s\n",rows,row[0].c_str(),row[1].c_str(),row[12].c_str(),trace.c_str());CHECK(trace==row[12]);CHECK(scene.calls==std::stoul(row[16]));CHECK(PopulationHex(PopulationMap(scene.map))==row[15]);
        if (row[12].find("create:")!=std::string::npos) { const auto raw=scene.pool.Slot({15002});CHECK(Get(raw,36)==std::stoul(row[13]) && Get(raw,26)==std::stoul(row[14])); } //거주구역전용/자연권한반환은생성되지않은새객체를읽지않는다.
        ++rows;
    } //모든Native행을처음부터실제복원몸체에재생한다.
    CHECK(rows==118);
}

TEST_CASE(IslandAdd_ActualPreviewCollisionAndPlaceBeforeMissingGeneration) {
    const std::vector<std::string> row{"Populate","0","7","1","1","0","71","133","1","1","0","639"};PopulationScene scene(row);ChunkMap preview(14,14);RawIslandBuilderState state{{0,0,14,14},8};std::uint32_t piece=107;bool fort=true;std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<FrameCode> codes;
    for (std::uint8_t side='A';side<='P';++side) for (std::uint8_t number=0;number<11;++number) codes.push_back({side,'P',number,0}); //독립기존Builder관측과같은완전프레임입력이다.
    frames[107]=RiftTypeFrames(std::move(codes));IslandAddHooks effects;std::uint32_t graph{},saves{},generation{};
    effects.generate=[&](const auto& current,auto region) { ++generation;CHECK(current.owner==1 && current.bounds.left==1 && current.bounds.top==1 && current.bounds.right==15 && current.bounds.bottom==15 && region==4);throw std::logic_error("actual-generation-stop"); };effects.graph=[&](auto) { ++graph;throw std::logic_error("미선택실제Graph"); };effects.territory=[&](auto) { ++saves;throw std::logic_error("미선택실제Territory저장"); };
    RawIslandAdd add(scene.map,*scene.islands,frames,piece,state,fort,*scene.population,[](auto) { throw std::logic_error("미선택Builder진단"); },std::move(effects));std::array<std::uint8_t,6> record{0,3,0,7,0xab,0xcd};const auto before=PopulationMap(scene.map);
    for (auto& entry:preview.Entries()) entry.island=0; //명시임시지도전체충돌이며실제world는변경하지않는다.
    CHECK(!add.TryDrop(preview,record) && record[1]==3 && state.owner==8 && scene.islands->Count()==4 && generation==0 && PopulationMap(scene.map)==before);
    preview.Reset();bool selected{};try { static_cast<void>(add.TryDrop(preview,record)); }catch (const std::logic_error& error) { selected=std::string_view(error.what())=="actual-generation-stop"; }
    CHECK(selected && generation==1 && graph==0 && saves==0 && scene.events.empty() && record[1]==1 && state.owner==1 && scene.islands->Count()==5 && scene.islands->Owner(4)==1);CHECK(std::any_of(scene.map.Entries().begin(),scene.map.Entries().end(),[](const auto& entry) { return entry.island==4; })); //실제Place/Runtime.Add만끝난부분효과를뒤하위미연결실패에서보존한다.
}
