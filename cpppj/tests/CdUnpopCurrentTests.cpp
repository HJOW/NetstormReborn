// CD 공간 해제의 최초 CPU 관측과 실제 World/Factory 종속 분리를 검사한다. 실제 플레이/MP는 실행하지 않는다.
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "o/RawSquidUnpopParent.h"
#include "o/RawSquidContainedUnpop.h"
#include "o/RawBuildingSpace.h"
#include "client/RawSurfaceWorld.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 최초 관측과 같은 명시 입력만 구성하며 결과를 만들지 않는다.
struct CdUnpopScene { // 같은 원시 풀·네 해시·전체 점유 지도를 관측한다.
    SidPool pool{OriginalEdition::Cd1072,24000};SquidHash hash;std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 원문 물리 자료다.
    std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(171);BuildingSpaceState state{83,82,false};RawBuildingSpace building{pool,hash,types,state}; // 옆면 WORD 쓰기는 기존 실제 몸체를 사용한다.
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
    explicit CdUnpopScene(const std::vector<std::string>& row) { // fixture의 명시 입력만 읽는다.
        checking=Number(row[1]);source=Number(row[2]);flags=Number(row[13]);action=Number(row[14]);std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(Number(row[15])));
        if (Number(row[15])&0x100U) for (std::size_t cell=0;cell<spots.size();++cell) spots[cell]=static_cast<std::uint8_t>(cell*37+Number(row[15])); // Native와 같은 비균일 초기 입력만 놓는다.
        for (std::uint32_t sid=0;sid<=128;++sid) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{}); } // CPU의 실제 관측 구간4644BYTE와 같은 초기 바이트다.
        Type(70,Number(row[11]),Number(row[10]),Number(row[8]),Number(row[9]));Type(6,Number(row[11]),Number(row[10]),Number(row[8]),Number(row[9]));Type(71,0x800,1,3,3);Type(72,0,6,2,2);Type(73,0,0,1,1);
        auto raw=Raw(source);Put(raw,0,0x61f000);raw[10]=static_cast<std::uint8_t>(Number(row[3]));raw[11]=static_cast<std::uint8_t>(Number(row[4]));raw[35]=static_cast<std::uint8_t>(Number(row[5]));Put(raw,14,Number(row[6]));Put(raw,18,Number(row[7]));Put(raw,4,51,2);Put(Raw(49),4,source,2);Raw(70)[10]=83;Put(Raw(70),12,0xa5ff,2);
        if (Number(row[16])) for (int y=17;y<=22;++y) for (int x=13;x<=18;++x) hash.Cell(0,x,y)=70; // 주변 표면 입력만 배치한다.
        const auto layout=Number(row[12]);if (layout==1 || layout==4) hash.Bucket(0,X(),Y())=static_cast<std::uint16_t>(source);if (layout==2 || layout==5) hash.Bucket(1,X(),Y())=49;if (layout==3 || layout==4) hash.Bucket(3,X(),Y())=static_cast<std::uint16_t>(source);
        if (layout==5) Put(Raw(source),4,49,2);if (layout==6 || layout==7) { hash.Bucket(1,X(),Y())=static_cast<std::uint16_t>(source);Put(Raw(source),4,layout==6 ? source : 65535,2); } // 직접 머리 저장은 next를 추가 역참조하지 않는다.
        hooks.type=[this](std::uint8_t number) -> const RiftTypeRecord& { return types.at(number); };hooks.assertion=[this](auto expression,auto file,auto line) { Assertion(expression,file,line); };
        hooks.miniMap=[this](Sid sid,std::uint32_t genus) { events+="M,"+std::to_string(sid.value)+','+std::to_string(Get(Raw(source),14))+','+std::to_string(Get(Raw(source),18))+','+std::to_string(genus)+';';if (action==4) { checking=0x80000000;Raw(source)[10]=71;Put(Raw(source),14,0x41880000);Put(Raw(source),18,0x41a80000);types[70].footX=types[70].footY=2; }if (action==5) Raw(source)[35]|=8;if (action==6) Raw(source)[11]|=8;if (action==12) checking=0; };
        hooks.side=[this](int x,int y) { const auto sid=building.Surface(x,y);building.UnpopSide(x,y);events+="N,"+std::to_string(sid.value)+';';if (notified) return;notified=true;if (action==7) { checking=0x80000000;Raw(source)[10]=71;Put(Raw(source),14,0x41880000);Put(Raw(source),18,0x41a80000); }if (action==9) Raw(source)[11]|=8; };
        hooks.contained=[this](Sid sid) { events+="D,"+std::to_string(sid.value)+";Q,"+std::to_string(sid.value)+';'; }; // 원문 관측에서도 두 요청은 명시 외부 경계이며 실제 하위 완료로 세지 않는다.
        hooks.update=[this](Sid sid,std::uint32_t value) { events+="U,"+std::to_string((value&0x2000U) ? 0x8cU : 0x88U)+','+std::to_string(sid.value)+','+std::to_string(Raw(sid.value)[10])+','+std::to_string(Raw(sid.value)[11])+';'; };
    }
    std::vector<std::string> Run() { // 원문과 같은 전체 물리 체크섬과 실제 요청을 내보낸다.
        RunSquidUnpopParent(pool,hash,spots,Sid{static_cast<std::uint16_t>(source)},flags,RawCheckingFlag{checking},hooks);std::vector<std::uint8_t> poolBytes,hashBytes;
        for (std::uint32_t sid=0;sid<=128;++sid) { const auto raw=Raw(sid);poolBytes.insert(poolBytes.end(),raw.begin(),raw.end()); } // 관측한 모든 raw BYTE를 합친다.
        for (int level=0;level<4;++level) for (const auto value:hash.Entries(level)) { hashBytes.push_back(static_cast<std::uint8_t>(value));hashBytes.push_back(static_cast<std::uint8_t>(value>>8)); } // WORD의 endian/단계 순서를 원문에 맞춘다.
        const auto raw=Raw(source);return {std::to_string(checking),std::to_string(raw[10]),std::to_string(raw[11]),std::to_string(raw[35]),std::to_string(Get(raw,14)),std::to_string(Get(raw,18)),std::to_string(types[70].footX),std::to_string(types[70].footY),std::to_string(Adler(poolBytes)),std::to_string(Adler(hashBytes)),std::to_string(Adler(spots)),events.empty() ? "-" : events};
    }
};
const auto& CdUnpopRows() { static const auto rows=LoadFixture(NETSTORM_CD_UNPOP_CURRENT_FIXTURE).rows;return rows; } // 최초 CPU1,402행을 읽기만 한다.
BridgeConnectState CdUnpopLinks() { BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;return links; } // 실제 World 설치에 필요한 정상 번호다.
}
TEST_CASE(CdUnpopCurrent_NativeParentSpaceAndSide1402) { // 전체 raw4644BYTE/네 해시172544BYTE/지도65536BYTE와 진단 요청을 최초 CPU에 대조한다.
    std::size_t count{};
    for (const auto& row:CdUnpopRows()) { // 첫 불일치는 입력/필드별 기대와 실제를 남긴다.
        CHECK(row.size()==29);CdUnpopScene scene(row);const auto actual=scene.Run();const std::vector<std::string> expected(row.begin()+17,row.end());CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t field=0;field<actual.size();++field) if (actual[field]!=expected[field]) std::printf("CD Unpop 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),field,expected[field].c_str(),actual[field].c_str());return; }++count; // Native 기대값은 제품으로 교체하지 않는다.
    }
    CHECK(count==1402);
}
TEST_CASE(CdUnpopCurrent_SelectedMissingDevicesKeepPartialEffects) { // 표시/종속 분리의 선택 실패는 앞선 void/지도/해시 쓰기를 남긴다.
    auto row=CdUnpopRows().front();row[8]=row[9]="1";row[10]="4";row[12]="1";row[16]="0";CdUnpopScene scene(row);Put(scene.Raw(50),4,0,2);scene.hooks.update={};CHECK(Throws([&] { scene.Run(); }));CHECK(scene.Raw(50)[11]==4 && scene.spots[20*256+16]==0x31 && scene.hash.Bucket(0,scene.X(),scene.Y())==0); // 이 경계 검사는 단일 항목 체인을 명시 입력으로 사용한다.
    CdUnpopScene contained(row);contained.Raw(50)[11]=8;contained.hooks.contained={};CHECK(Throws([&] { contained.Run(); }));CHECK(contained.Raw(50)[11]==12 && contained.spots[20*256+16]==0x35 && contained.events.find("A,1472:")==0);
    row[12]="0";CdUnpopScene notFound(row);notFound.Run();CHECK(notFound.Raw(50)[11]==4 && notFound.events.find("A,")==std::string::npos && notFound.events.find("U,136,50,")!=std::string::npos); // CD에는 PATCH의 found/spot 진단이 없다.
}
TEST_CASE(CdUnpopCurrent_ActualWorldCurrentDiagnosticAndContainedDetach) { // 현재 TYPE/진단의 재대여와 실제 CD Factory Take→종속 분리에 도달한다.
    auto types=netstorm::test::MakeFormTypeInputs(171);types[74].footX=types[74].footY=types[75].footX=types[75].footY=1;types[74].flags2=types[75].flags2=4;std::vector<RiftTypeFrames> frames(171,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(171);auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Cd1072,types,frames,shapes);std::optional<std::string> detail;RawWorldCore* current{};Sid victim{};std::uint32_t variant{},diagnostics{};
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(current && file=="\\ns\\o\\Squid.cpp");CHECK(expression==(variant==0 ? "!isForm()" : "!isContained()") && line==(variant==0 ? 0x5bfU : 0x5c0U));++diagnostics;if (variant<2) { auto raw=current->pool.AllocatedBytes(victim);raw[10]=75;raw[11]=0; } }); // CD 진단이 돌아온 뒤 현재 TYPE/상태를 다시 읽는다.
    for (std::uint32_t generation=0;generation<2;++generation) { // 같은 장기 Storage의 서비스로 새 World 대여도 처리한다.
        for (variant=0;variant<3;++variant) { // Form/contained 접두 진단과 실제 종속 분리를 별도 입력으로 확인한다.
            RawSurfaceWorld world(OriginalEdition::Cd1072,storage->types,frames,shapes,CdUnpopLinks(),83,nullptr,storage);SurfaceDisplayHooks display;display.suppressed=[] { return true; };world.SetDisplay({},std::move(display));auto core=world.Core();current=&core;storage->types[74].flags2=variant==2 ? 0U : 4U;victim=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(victim);raw[10]=variant==0 ? 6 : 74;raw[11]=variant==0 ? 0 : 8;Sid parent{};
            if (variant==2) { parent=core.factory.Create(74);Put(raw,14,parent.value,2);Put(raw,16,0,2);Put(raw,4,0,2);Put(core.pool.AllocatedBytes(parent),6,victim.value,2); }
            else { Put(raw,14,std::bit_cast<std::uint32_t>(16.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));core.hash.Bucket(1,16,20)=victim.value;core.spots[20*256+16]=0x35; }
            const auto before=diagnostics,free=core.pool.FreeCount();if (variant==2) CHECK(core.factory.Take(74,victim)==victim);else core.unpop.Unpop(victim,core.types[74]);CHECK(diagnostics==before+1 && raw[11]==4 && core.pool.FreeCount()==free);
            if (variant==2) CHECK(raw[10]==74 && Get(core.pool.Slot(parent),6,2)==0);else CHECK(raw[10]==75 && core.spots[20*256+16]==0x31 && core.hash.Bucket(1,16,20)==0);current=nullptr;
        }
    }
}
TEST_CASE(CdUnpopCurrent_ContainedDiagnosticPathMatchesProtectedPe) { // 공통 분리의 CD 원문 경로/행과 진단 복귀 뒤 현재 dead를 확인한다.
    SidPool pool(OriginalEdition::Cd1072,32768);auto types=netstorm::test::MakeFormTypeInputs(171);SquidFactory factory(pool,types);const auto parent=factory.Create(74),form=factory.CreateGumpForm(0);auto raw=pool.AllocatedBytes(form);Put(raw,16,parent.value,2);raw[11]=0;std::uint32_t content=6,gump=3,event=0;std::size_t calls{};SquidContainedUnpopHooks hooks;
    hooks.parent=[&](Sid sid) { return CommonContainedParent(pool,sid); }; // 원문 현재 getter의 부모를 사용한다.
    hooks.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="containedFlag" && file=="\\ns\\o\\Squid.cpp" && line==0x39aU);++calls;raw[11]=8;pool.AllocatedBytes(parent)[11]=2; }; // 진단 이후 부모 dead 분기는 링크를 보존한다.
    RawSquidContainedUnpop(pool,{content,gump,event,{}, {}, {}},std::move(hooks)).Detach(form);CHECK(calls==1 && raw[11]==8);
}
