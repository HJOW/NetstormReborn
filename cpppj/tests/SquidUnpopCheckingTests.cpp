// 실제CPU 공간 변경 전체와 현재 Client/World의 진단 복귀를 대조한다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "RawSceneSupport.h"
#include "o/RawSquidUnpopParent.h"
#include "o/RawBuildingSpace.h"
#include "client/RawSurfaceWorld.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 부모 결과를 만들지 않고 원문과 같은 외부 입력만 공급한다.
struct UnpopCheckingScene { // 같은 원시 풀·네 해시·전체 점유 지도를 관측한다.
    SidPool pool{OriginalEdition::Patch1078,24000};SquidHash hash;std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 원문 물리 자료다.
    std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);BuildingSpaceState state{83,82,false};RawBuildingSpace building{pool,hash,types,state}; // 옆면 BYTE 쓰기는 기존 실제 몸체를 사용한다.
    std::uint32_t source{},checking{},flags{},action{};bool notified{};std::string events;SquidUnpopParentHooks hooks; // 현재 입력/명시 경계/실제 요청이다.
    std::span<std::uint8_t> Raw(std::uint32_t sid) { const auto raw=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 예약0/진단 뒤 상태도 물리 슬롯으로 읽고 쓴다.
    void Type(std::uint32_t number,std::uint32_t first,std::uint32_t genus,std::uint32_t width,std::uint32_t height) { auto& type=types[number];type.flags1=first;type.flags2=genus;type.footX=static_cast<int>(width);type.footY=static_cast<int>(height); } // 원문 TYPE 입력만 구성한다.
    float X() { return std::bit_cast<float>(Get(Raw(source),14)); } // 현재 raw 위치다.
    float Y() { return std::bit_cast<float>(Get(Raw(source),18)); } // 현재 raw 위치다.
    void Assertion(std::string_view expression,std::string_view file,std::uint32_t line) { // 원문과 같은 진단 정상 복귀 입력이다.
        events+="A,"+std::to_string(line)+':'+std::string(expression)+':'+std::string(file)+';';auto raw=Raw(source);
        if (action==1 && line==0x617) { checking=0;std::fill(spots.begin(),spots.end(),std::uint8_t{0xf0}); }
        if (action==2) { checking=0x80000000;raw[10]=71; }
        if (action==3) { checking=0;raw[10]=70;raw[11]=0; }
        if (action==8) { checking=0;raw[10]=73; }
        if (action==10 && line==0x65d) hash.Bucket(0,X(),Y())=static_cast<std::uint16_t>(source);
        if (action==11) { checking=0x80000000;raw[10]=70;raw[11]&=0xf7; }
    }
    explicit UnpopCheckingScene(const std::vector<std::string>& row) { // fixture의 명시 입력만 읽는다.
        checking=Number(row[1]);source=Number(row[2]);flags=Number(row[13]);action=Number(row[14]);std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(Number(row[15])));
        for (std::uint32_t sid=0;sid<=128;++sid) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{}); } // CPU의 실제 관측 구간6450BYTE와 같은 초기 바이트다.
        Type(70,Number(row[11]),Number(row[10]),Number(row[8]),Number(row[9]));Type(6,Number(row[11]),Number(row[10]),Number(row[8]),Number(row[9]));Type(71,0x800,1,3,3);Type(72,0,6,2,2);Type(73,0,0,1,1);
        auto raw=Raw(source);Put(raw,0,0x61f000);raw[10]=static_cast<std::uint8_t>(Number(row[3]));raw[11]=static_cast<std::uint8_t>(Number(row[4]));raw[40]=static_cast<std::uint8_t>(Number(row[5]));Put(raw,14,Number(row[6]));Put(raw,18,Number(row[7]));Put(raw,4,51,2);Put(Raw(49),4,source,2);Raw(70)[10]=83;Raw(70)[12]=0xff;
        if (Number(row[16])) for (int y=17;y<=22;++y) for (int x=13;x<=18;++x) hash.Cell(0,x,y)=70; // 주변 표면 입력만 배치한다.
        const auto layout=Number(row[12]);if (layout==1 || layout==4) hash.Bucket(0,X(),Y())=static_cast<std::uint16_t>(source);if (layout==2) hash.Bucket(1,X(),Y())=49;if (layout==3 || layout==4) hash.Bucket(3,X(),Y())=static_cast<std::uint16_t>(source);
        hooks.type=[this](std::uint8_t number) -> const RiftTypeRecord& { return types.at(number); };hooks.assertion=[this](auto expression,auto file,auto line) { Assertion(expression,file,line); };
        hooks.miniMap=[this](Sid sid,std::uint32_t genus) { events+="M,"+std::to_string(sid.value)+','+std::to_string(Get(Raw(source),14))+','+std::to_string(Get(Raw(source),18))+','+std::to_string(genus)+';';if (action==4) { checking=0x80000000;Raw(source)[10]=71;Put(Raw(source),14,0x41880000);Put(Raw(source),18,0x41a80000);types[70].footX=types[70].footY=2; }if (action==5) Raw(source)[40]|=8;if (action==6) Raw(source)[11]|=8;if (action==12) checking=0; };
        hooks.side=[this](int x,int y) { const auto sid=building.Surface(x,y);building.UnpopSide(x,y);events+="N,"+std::to_string(sid.value)+';';if (notified) return;notified=true;if (action==7) { checking=0x80000000;Raw(source)[10]=71;Put(Raw(source),14,0x41880000);Put(Raw(source),18,0x41a80000); }if (action==9) Raw(source)[11]|=8; };
        hooks.contained=[this](Sid sid) { events+="D,"+std::to_string(sid.value)+";Q,"+std::to_string(sid.value)+';'; }; // 원문 관측에서도 두 요청은 명시 외부 경계이며 실제 하위 완료로 세지 않는다.
        hooks.update=[this](Sid sid,std::uint32_t value) { events+="U,"+std::to_string((value&0x2000U) ? 0x8cU : 0x88U)+','+std::to_string(sid.value)+','+std::to_string(Raw(sid.value)[10])+','+std::to_string(Raw(sid.value)[11])+';'; };
    }
    std::vector<std::string> Run() { // 원문과 같은 전체 물리 체크섬과 실제 요청을 내보낸다.
        RunSquidUnpopParent(pool,hash,spots,Sid{static_cast<std::uint16_t>(source)},flags,RawCheckingFlag{checking},hooks);std::vector<std::uint8_t> poolBytes,hashBytes;
        for (std::uint32_t sid=0;sid<=128;++sid) { const auto raw=Raw(sid);poolBytes.insert(poolBytes.end(),raw.begin(),raw.end()); } // 관측한 모든 raw BYTE를 합친다.
        for (int level=0;level<4;++level) for (const auto value:hash.Entries(level)) { hashBytes.push_back(static_cast<std::uint8_t>(value));hashBytes.push_back(static_cast<std::uint8_t>(value>>8)); } // WORD의 endian/단계 순서를 원문에 맞춘다.
        const auto raw=Raw(source);return {std::to_string(checking),std::to_string(raw[10]),std::to_string(raw[11]),std::to_string(raw[40]),std::to_string(Get(raw,14)),std::to_string(Get(raw,18)),std::to_string(types[70].footX),std::to_string(types[70].footY),std::to_string(Adler(poolBytes)),std::to_string(Adler(hashBytes)),std::to_string(Adler(spots)),events.empty() ? "-" : events};
    }
};
const auto& UnpopCheckingRows() { static const auto rows=LoadFixture(NETSTORM_SQUID_UNPOP_CHECKING_FIXTURE).rows;return rows; } // 최초 CPU1300행을 읽기 전용으로 쓴다.
}
TEST_CASE(SquidUnpopChecking_NativeParentAndActualSide1300) { // 전체 raw6450BYTE/네 해시172544BYTE/지도65536BYTE를 대조한다.
    std::size_t count{};for (const auto& row:UnpopCheckingRows()) { // 첫 불일치의 원문 입력과 각 필드를 남긴다.
        CHECK(row.size()==29);UnpopCheckingScene scene(row);const auto actual=scene.Run();const std::vector<std::string> expected(row.begin()+17,row.end());CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("Unpop 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),index,expected[index].c_str(),actual[index].c_str());return; }++count; // 기대값은 제품에서 만들지 않는다.
    }CHECK(count==1300);
}
TEST_CASE(SquidUnpopChecking_SelectedDiagnosticPreservesPartialEffects) { // 체인 실패 진단 누락도 이미 실행한 void/spot 쓰기를 남긴다.
    auto row=UnpopCheckingRows().front();row[10]="4";row[8]=row[9]="1";row[12]="0";row[16]="0";UnpopCheckingScene scene(row);scene.hooks.assertion={};CHECK(Throws([&] { scene.Run(); }));CHECK(scene.Raw(50)[11]==4 && scene.spots[20*256+16]==0x31 && scene.events.find("M,50,")==0 && scene.events.find("U,")==std::string::npos);
    UnpopCheckingScene missingContained(row);missingContained.Raw(50)[11]=8;missingContained.hooks.contained={};missingContained.hooks.assertion=[](auto,auto,auto) {};CHECK(Throws([&] { missingContained.Run(); }) && missingContained.Raw(50)[11]==12); // 미연결 종단을 성공으로 대체하지 않는다.
}
TEST_CASE(SquidUnpopChecking_ActualClientWorldCurrentDiagnosticAndSavedSpot) { // 실제 두 World 대여와 같은 Client 현재 DWORD/서비스를 사용한다.
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);types[74].footX=types[74].footY=types[75].footX=types[75].footY=1;types[74].flags2=4;types[75].flags2=1;std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);storage->BindChecking(client.GameplayGlobals()->AiDebug().Checking());std::optional<std::string> detail;RawWorldCore* current{};Sid victim{};std::uint32_t variant{},diagnostics{};
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(current && file=="\\Ns\\O\\Squid.cpp");CHECK(line==(variant==0 ? 0x617U : variant==1 ? 0x629U : 0x65dU));CHECK(expression==(variant==0 ? "(flags & effectiveGenus) == ( unsigned char)effectiveGenus" : variant==1 ? "getGridSid( (float)fx, (float)fy) == getSid()" : "found"));++diagnostics;client.GameplayGlobals()->AiDebug().Checking()=0;current->pool.AllocatedBytes(victim)[10]=75;if (!variant) current->spots[20*256+16]=0xf0; });
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    for (std::uint32_t generation=0;generation<2;++generation) for (variant=0;variant<3;++variant) { // 다시 대여 후에도 같은 진단 주소와 실제 공간을 사용한다.
        RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();current=&core;storage->types[74].flags1=variant==1 ? 0x800U : 0U;victim=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(victim);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x501dd0);raw[10]=74;Put(raw,14,std::bit_cast<std::uint32_t>(16.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));core.hash.Reset();if (variant<2) core.hash.Bucket(1,16,20)=victim.value;if (variant==1) core.hash.Cell(0,16,20)=70;core.spots[20*256+16]=variant==0 ? 0U : 0x35U;client.GameplayGlobals()->AiDebug().Checking()=variant==2 ? 0 : std::bit_cast<std::int32_t>(0x80000000U);const auto before=diagnostics;core.unpop.Unpop(victim,core.types[74]);CHECK(diagnostics==before+1 && !client.GameplayGlobals()->AiDebug().Checking() && raw[10]==75 && (raw[11]&4U));CHECK(core.spots[20*256+16]==(variant==0 ? 0U : 0x31U) && core.hash.Bucket(1,16,20)==0);current=nullptr;
    }
}
