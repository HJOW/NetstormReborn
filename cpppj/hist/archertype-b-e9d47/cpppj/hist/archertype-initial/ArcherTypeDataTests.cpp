// 보호 TYPE/실제SHP와 원본 callback의 방향수·물리프레임·특수프레임을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawArcherTypeData.h"
#include "o/RawBuildingSpace.h"
#include "o/RawGunTarget.h"
#include "client/SquidRenderer.h"
#include <cstdio>
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 기대값은 원본 callback/후처리와 parser 접두의 저장된 관찰만 읽는다.
const std::vector<std::vector<std::string>>& ArcherTypeRows() { static const auto rows=LoadFixture(NETSTORM_ARCHERTYPE_FIXTURE).rows;return rows; }
// 원본 AST에 공급한 ASCII 투영문을16진수 입력에서 복원한다.
std::string ArcherTypeText(std::string_view value) {
    std::string text;
    // 두 자리씩 읽고 기대 결과 칸은 참조하지 않는다.
    for (std::size_t offset=0;offset<value.size();offset+=2) text+=static_cast<char>(std::stoul(std::string(value.substr(offset,2)),nullptr,16));
    return text;
}
// 원본 현재TYPE 의미 필드와 기존 실제4바이트 프레임/16문자Run 조회를 사용한다.
std::vector<std::string> ArcherTypeProjection(const RiftTypeDefinition& definition,std::size_t shapeCount,bool prefix) {
    const auto data=ReadRawArcherTypeData(definition);const auto table=definition.FrameTable();const auto& special=definition.specialFrames;
    const std::array<std::int32_t,8> values{data.directions,data.frameCount,special.defaultFrame,special.gumpFrame,special.helpFrame,special.baseFrame,prefix ? 0 : table.LetterKinds(),static_cast<std::int32_t>(shapeCount)};
    std::string metadata,codes,runs;
    // 원본의 의미 필드를 같은 순서로 직렬화한다.
    for (const auto value:values) { if (!metadata.empty()) metadata+=',';metadata+=std::to_string(value); }
    if (prefix) return {metadata,"-","-"};
    // 실제4바이트 코드 배열 전체를 그대로 비교한다.
    for (const auto& code:table.Codes()) { const std::array<std::uint8_t,4> bytes{code.side,code.variant,code.number,code.flags};codes+=Hex(bytes); }
    // first 배열16개 뒤 count 배열16개를 원본 순서로 비교한다.
    for (const bool first:{true,false}) for (std::uint8_t letter='A';letter<='P';++letter) {
        const auto run=table.Run(letter);if (!runs.empty()) runs+=',';runs+=std::to_string(first ? static_cast<std::uint32_t>(run.first) : static_cast<std::uint32_t>(run.count));
    }
    return {metadata,codes,runs};
}
// 두 판본의 같은 보호 자산을 읽어 실제35프레임과 모든 독립 AST 투영 입력을 대조한다.
void ReplayArcherType(std::string_view edition) {
    const auto original=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/edition;BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");netstorm::client::GameAssets game(files,original);std::size_t count=0;int failures=0;const auto& asset=game.Find("sunArcher");const auto shapeCount=game.Shapes().Blocks()[asset.block].frames.size();
    CHECK(shapeCount==35 && !asset.definition.Number("orientations").has_value());
    // 각 행의 input 투영문과 실제PE 출력만 사용한다.
    for (const auto& row:ArcherTypeRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);const bool prefix=row[1]=="prefix";RiftTypeDefinition definition;
        if (!prefix) definition=RiftTypeDefinition::Parse(ArcherTypeText(row[4]));const auto output=ArcherTypeProjection(definition,prefix ? 0 : shapeCount,prefix);const auto expected=std::vector<std::string>(row.begin()+5,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초 불일치의 원본 값과 C++ 값을 함께 남긴다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s ArcherType 행%zu/%s 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,row[1].c_str(),index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==22);
}
// 실제 SHP 변경영역을 수집하며 창/그래픽 장치를 만들지 않는다.
class ArcherTypeDisplay final:public SquidDisplaySink {
public:
    // 원본 표시 억제를 해제하여 실제 프레임 경계를 계산한다.
    bool Suppressed() const override { return false; }
    // 실제 공유표시가 만든 화면 변경 요청을 보존한다.
    void Invalidate(SquidDisplayRect rectangle,std::uint32_t) override { rectangles.push_back(rectangle); }
    std::vector<SquidDisplayRect> rectangles; // 현재 실제 SHP/좌표의 변경 사각형이다.
};
}
// 패치188TYPE의 초기화 접두1/전체callback+후처리21을 실제 현재 소비자와 비교한다.
TEST_CASE(ArcherTypeData_PatchNativeMetadataAndInitPrefix) { ReplayArcherType("originals"); }
// CD10.72의171TYPE/별도초기화body와 실제35코드/방향수0을 비교한다.
TEST_CASE(ArcherTypeData_CdNativeMetadataAndInitPrefix) { ReplayArcherType("originalCD"); }
// 실제 sunArcher의 group0/defaultP32/방향0을 같은 Host가 회전생략→발사 생성→첫프레임으로 소비한다.
TEST_CASE(ArcherTypeData_ActualAssetsGunSkipsRotateAndStartsFire) {
    // 실제 양판본 보호 TYPE/SHP와 같은 풀·공간·표시·Form·Kernel을 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(patch ? "originals" : "originalCD");BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");netstorm::client::GameAssets game(files,edition);const auto types=game.TypeTable().Types();const auto& definition=game.Find("sunArcher").definition;std::vector<ArcherTypeData> data(types.size());data[71]=ReadRawArcherTypeData(definition);CHECK(data[71].directions==0 && data[71].frameCount==35 && definition.specialFrames.defaultFrame==32 && types[71].group==0);
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);auto shapes=netstorm::client::SquidRenderer::Shapes(game,edition);ArcherTypeDisplay sink;SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display);BuildingSpaceState buildingState;buildingState.islandType=94;buildingState.bridgeType=82;RawBuildingSpace building(pool,hash,types,buildingState);unpop.SetBuildingSpace(&building);pop.SetBuildingSpace(&building);SquidFactory factory(pool,types,false,&unpop);
        // 원본과 같은 번호를 확보한 뒤 실제 생성자의 vtable/HP 쓰기를 소비한다.
        for (std::uint16_t id=5;id<50;++id) CHECK(pool.Allocate(2)==Sid{id});
        CHECK(factory.Create(71,2)==kSource);
        // 중간 슬롯은 검사 장면의 번호 배치이며 게임 진행으로 세지 않는다.
        for (std::uint16_t id=51;id<60;++id) CHECK(pool.Allocate(2)==Sid{id});
        const auto target=factory.Create(158,2);CHECK(target==Sid{60});auto parent=pool.AllocatedBytes(kSource);Put(parent,patch ? 36U : 34U,static_cast<std::uint32_t>(definition.specialFrames.defaultFrame),patch ? 4U : 1U);
        // 현재fort/default 선택 경계의 P32를 실제 SHP 크기/공간으로 등록한다.
        const auto size=display.FrameSize(edition,parent,definition.specialFrames.defaultFrame);CHECK(pop.Pop(kSource,types[71],size[0],size[1],50.5f,50.5f)==RawPopResult::Registered);auto rawTarget=pool.AllocatedBytes(target);Put(rawTarget,14,std::bit_cast<std::uint32_t>(60.5f));Put(rawTarget,18,std::bit_cast<std::uint32_t>(60.5f));
        RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState hostState;hostState.now=10;std::array<std::uint8_t,64> packet{};std::vector<std::vector<std::uint8_t>> packets;ProcessFormTransmission transmit;transmit.writeData=[&](SquidProcess& process,bool changed) { auto* gun=dynamic_cast<GunProcess*>(&process);if (!gun) throw std::logic_error("실제 ArcherType Gun writer 타입 오류");return static_cast<std::int32_t>(gun->WritePacket(packet,changed)); };transmit.transmit=[&](std::int32_t length) { packets.emplace_back(packet.begin(),packet.begin()+length); };SquidProcessHost host(pool,types,kernel,destroy,hostState,{}, {},transmit);
        std::vector<GunTargetType> targetTypes(types.size());GunTargetState targetState;RawGunTarget geometry(pool,hash,types,targetTypes,shapes,targetState);ArcherAnimationState animationState;animationState.now=10;animationState.delta=0.1;ArcherAnimationHooks animationHooks;animationHooks.common.center=[&](Sid sid) { return geometry.Center(sid); };animationHooks.common.unpop=[&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,types[pool.Slot(sid)[10]],flags); };animationHooks.common.repop=[&](Sid sid,std::uint32_t flags) { const auto raw=pool.Slot(sid);const auto frame=static_cast<std::int32_t>(Get(raw,patch ? 36U : 34U,patch ? 4U : 1U));const auto dimensions=display.FrameSize(edition,raw,frame);CHECK(pop.Pop(sid,types[raw[10]],dimensions[0],dimensions[1],std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),flags)==RawPopResult::Registered); };animationHooks.notifySurface=[&](Sid sid) { building.NotifySurface(sid); };RawArcherAnimation animation(pool,types,data,animationState,MakeArcherAnimationHostHooks(host,std::move(animationHooks)));GunProcessState gunState;gunState.now=10;RawGunProcess gunLogic(pool,types,gunState,MakeArcherAnimationProcessHooks(host,animation,{}));auto* gun=CreateGunProcess(host,gunLogic,kSource);CHECK(gun!=nullptr);if (!gun) throw std::logic_error("실제 ArcherType Gun 생성 실패");Put(gun->Fields().bytes,24,target.value,2);Put(gun->Fields().bytes,28,2);
        gun->RunFrame();CHECK(gun->Fields().Stage()==4 && kernel.Size()==1);gun->RunFrame();CHECK(gun->Fields().Stage()==5 && kernel.Size()==1);gun->RunFrame();CHECK(gun->Fields().Stage()==6 && kernel.Size()==2);auto* fire=dynamic_cast<ArcherAnimation*>(host.FromForm(Sid{static_cast<std::uint16_t>(Get(pool.Slot(kSource),6,2))}));CHECK(fire!=nullptr);if (!fire) throw std::logic_error("실제 방향0 Archer Fire 조회 실패");CHECK(Get(fire->Fields().bytes,64)==std::bit_cast<std::uint32_t>(0.0f));fire->RunFrame();CHECK(gun->Fields().Stage()==6 && Get(pool.Slot(kSource),patch ? 36U : 34U,patch ? 4U : 1U)<35 && !sink.rectangles.empty() && !packets.empty());host.Kill(*fire,0x10);host.Kill(*gun,0x40);CHECK(kernel.Size()==0 && destroy.PreDepth()==0 && destroy.PostDepth()==0);
    }
}
