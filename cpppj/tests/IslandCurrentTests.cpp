// PATCH 표면의 현재 물리/선택 장치/진단 복귀를 원문 호출 순서로 대조한다. 게임·프레임·통신은 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawIslandPostPop.h"
#include "o/RawGameAuxState.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 실제 고정/현재 지도와 타입 항목을 분리한 작은 입력이다.
struct SurfaceCurrentScene { // 구성 뒤에도 지도/현재 전역/프레임 항목을 콜백에서 바꿀 수 있다.
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 할당 상태와 독립인 물리 슬롯 쓰기를 확인한다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 타입 항목의 주소는 검사 중 유지한다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); // 현재 코드 표를 매 getter에서 읽는다.
    std::vector<std::uint16_t> current=std::vector<std::uint16_t>(65536),fixed=std::vector<std::uint16_t>(65536); // 서로 다른 원문 두 지도다.
    BridgeConnectState links;IslandPostPopState state; // 현재 TYPE와 로딩/저장 게임 관문이다.
    SurfaceCurrentScene() { // 일반 A 칸을 만들며 도달하지 않은 외부 장치는 설치하지 않는다.
        links.islandType=91;links.stalagType=92;links.noIslandType=93;state.ownerPropagationSuppressed=true;
        auto raw=Slot(50);raw[10]=93;raw[34]=2;Put(raw,14,std::bit_cast<std::uint32_t>(30.0F));Put(raw,18,std::bit_cast<std::uint32_t>(30.0F));Put(raw,36,4);
        current[30*256+30]=fixed[30*256+30]=50;
        std::vector<FrameCode> codes;for (const char letter:std::string_view("FCGBADIEH")) codes.push_back({static_cast<std::uint8_t>(letter),'P',1,0}); // 실제 3×3 글자 순서다.
        frames[93]=RiftTypeFrames(std::move(codes));
    }
    std::span<std::uint8_t> Slot(std::uint16_t sid) { const auto raw=pool.Slot(Sid{sid});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 원문 WORD 번호의 물리 슬롯을 대여하며 할당을 추가하지 않는다.
    RawIslandPostPop Body(IslandPostPopHooks hooks) { return RawIslandPostPop(pool,current,types,frames,links,state,std::move(hooks),fixed); } // 같은 현재 입력을 실행기에 빌린다.
};
}
TEST_CASE(IslandCurrent_NonFirstSkipsInvalidGlobalsAndAllDevices) { // 최초가 아니면 슬롯의 현재 타입/수명이나 미설치 장치를 읽지 않는다.
    SurfaceCurrentScene s;s.links.islandType=s.links.stalagType=s.links.noIslandType=0xffffffffU;s.Slot(50)[10]=255;s.Slot(50)[11]=9;s.state.loadingDepth=1;auto body=s.Body({});const auto before=Hex(s.Slot(50));
    for (const auto flags:{0U,2U,4U}) body.Prefix(Sid{50},flags); // 세 비최초 입력 모두 아무 물리도 쓰지 않는다.
    CHECK(Hex(s.Slot(50))==before);
}
TEST_CASE(IslandCurrent_OrdinaryLetterNeedsOnlyReachedConnection) { // A 칸에서 잘못된 받침 전역과 미설치 생성/표시 장치를 요구하지 않는다.
    SurfaceCurrentScene s;s.links.islandType=0xffffffffU;s.links.stalagType=188;s.Slot(50)[11]=9;std::uint32_t connected{};IslandPostPopHooks hooks;hooks.connect=[&](Sid sid) { CHECK(sid==Sid{50});++connected; };auto body=s.Body(hooks);body.Prefix(Sid{50},1);CHECK(connected==1 && s.Slot(50)[11]==9);
}
TEST_CASE(IslandCurrent_LoadingStoresPhysicalFrameBeforeLateDeviceFailure) { // free/contained 물리에도 첫 프레임을 직접 저장한 뒤 도달한 생성 실패가 난다.
    SurfaceCurrentScene s;s.state.loadingDepth=1;s.Slot(50)[11]=9;auto body=s.Body({});CHECK(Throws([&] { body.Prefix(Sid{50},1); }));CHECK(Get(s.Slot(50),36)==0 && s.Slot(50)[11]==9);
}
TEST_CASE(IslandCurrent_RequiredFrameDiagnosisReturnsMinusOneWithoutRetry) { // 필수 검색 진단이 표와 frame을 고쳐도 원래 -1 반환을 저장한 뒤 실제 코드 범위에서 실패한다.
    SurfaceCurrentScene s;s.state.loadingDepth=1;s.frames[93]=RiftTypeFrames({});std::uint32_t calls{};IslandPostPopHooks hooks;hooks.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="0" && file==R"(\Ns\O\Rifttype.cpp)" && line==299 && Get(s.Slot(50),36)==4);++calls;s.frames[93]=RiftTypeFrames(std::vector<FrameCode>{{'F','P',1,0}});Put(s.Slot(50),36,0); };auto body=s.Body(hooks);CHECK(Throws([&] { body.Prefix(Sid{50},1); }));CHECK(calls==1 && Get(s.Slot(50),36)==0xffffffffU);
}
TEST_CASE(IslandCurrent_OrientationDiagnosisRereadsAndContinuesAfterMismatch) { // 로딩 저장 뒤 !isForm 진단이 현재 항목을 바꾸면 비교 진단을 거쳐 현재 A에서 연결한다.
    SurfaceCurrentScene s;s.state.loadingDepth=1;s.Slot(50)[10]=56;s.frames[56]=s.frames[93];std::vector<std::string> events;IslandPostPopHooks hooks;
    hooks.assertion=[&](auto expression,auto file,auto line) { events.emplace_back(expression);if (expression=="!isForm()") { CHECK(file==R"(\Ns\O\Squid.cpp)" && line==0x854 && Get(s.Slot(50),36)==0);s.Slot(50)[10]=93;Put(s.Slot(50),36,1); }else { CHECK(expression=="getOrientation() == orientation" && file==R"(\Ns\O\Construction.cpp)" && line==0xfa);Put(s.Slot(50),36,4); } };
    hooks.connect=[&](Sid) { events.emplace_back("connect"); };auto body=s.Body(hooks);body.Prefix(Sid{50},1);CHECK(events==std::vector<std::string>({"!isForm()","getOrientation() == orientation","connect"}));
}
TEST_CASE(IslandCurrent_LoadingRereadsCoordinatesAfterWestDiagnosis) { // 서쪽 getter 진단이 위치를 바꾸면 북쪽은 새 위치의 현재 지도를 조회한다.
    SurfaceCurrentScene s;s.state.loadingDepth=1;s.links.noIslandType=56;s.frames[56]=RiftTypeFrames(std::vector<FrameCode>{{'F','P',1,0},{'Z','P',1,0}});s.fixed[30*256+29]=60;s.Slot(60)[10]=56;Put(s.Slot(60),36,1);s.fixed[39*256+40]=61;s.Slot(61)[10]=56;Put(s.Slot(61),36,0);std::uint32_t diagnoses{};IslandPostPopHooks hooks;
    hooks.assertion=[&](auto expression,auto,auto line) { CHECK(expression=="!isForm()" && line==0x854);++diagnoses;if (diagnoses==1) { Put(s.Slot(50),14,std::bit_cast<std::uint32_t>(40.0F));Put(s.Slot(50),18,std::bit_cast<std::uint32_t>(40.0F)); } };hooks.connect=[](Sid) {};auto body=s.Body(hooks);body.Prefix(Sid{50},1);CHECK(diagnoses==2 && Get(s.Slot(50),36)==3); // 서쪽 Z 뒤 새 위치의 북쪽 F를 읽어 B를 선택한다.
}
TEST_CASE(IslandCurrent_CreateThenCaptureTypeBeforeOwnerAndReadFieldsAfterOwner) { // 생성자가 바꾼 현재 TYPE 항목 주소를 고정한 뒤 소유자 콜백의 필드/좌표 변경을 읽는다.
    SurfaceCurrentScene s;Put(s.Slot(50),36,0);std::vector<std::string> events;IslandPostPopHooks hooks;
    hooks.objects.create=[&](auto type,auto flags) { CHECK(type==91 && flags==2);events.emplace_back("create");s.links.islandType=94;s.Slot(50)[34]=5;return Sid{100}; };
    hooks.objects.setOwner=[&](auto sid,auto owner) { CHECK(sid==Sid{100} && owner==5);events.emplace_back("owner");s.types[94].footX=7;s.types[94].footY=9;s.links.islandType=95;s.types[95].footX=s.types[95].footY=100;Put(s.Slot(50),14,std::bit_cast<std::uint32_t>(40.0F));Put(s.Slot(50),18,std::bit_cast<std::uint32_t>(50.0F)); };
    hooks.objects.pop=[&](auto sid,float x,float y,auto flags) { CHECK(sid==Sid{100} && x==46 && y==58 && flags==0);events.emplace_back("pop"); };hooks.connect=[&](Sid) { events.emplace_back("connect"); };auto body=s.Body(hooks);body.Prefix(Sid{50},1);CHECK(events==std::vector<std::string>({"create","owner","pop","connect"}));
}
TEST_CASE(IslandCurrent_InvalidCurrentTypeFailsAfterCreationBeforeOwner) { // 생성 뒤 현재 TYPE 역참조가 범위를 벗어나면 이미 만든 효과는 남고 소유자는 호출하지 않는다.
    SurfaceCurrentScene s;Put(s.Slot(50),36,0);std::uint32_t created{};IslandPostPopHooks hooks;hooks.objects.create=[&](auto,auto) { ++created;s.links.islandType=188;return Sid{100}; };auto body=s.Body(hooks);CHECK(Throws([&] { body.Prefix(Sid{50},1); }) && created==1);
}
TEST_CASE(IslandCurrent_FootprintDecrementWrapsBeforeCoordinateAddition) { // 기계어 DEC의 INT_MIN 순환을 먼저 적용하며 잘못된 큰 음수 좌표를 만들지 않는다.
    SurfaceCurrentScene s;Put(s.Slot(50),36,0);s.types[91].footX=std::numeric_limits<std::int32_t>::min();s.types[91].footY=2;Put(s.Slot(50),14,std::bit_cast<std::uint32_t>(-2147483648.0F));std::uint32_t popped{};IslandPostPopHooks hooks;hooks.objects.create=[](auto,auto) { return Sid{100}; };hooks.objects.setOwner=[](auto,auto) {};hooks.objects.pop=[&](auto,float x,float y,auto) { CHECK(x==-1 && y==31);++popped; };hooks.connect=[](Sid) {};auto body=s.Body(hooks);body.Prefix(Sid{50},1);CHECK(popped==1);
}
TEST_CASE(IslandCurrent_MissingUpperLeftDiagnosisReturnsThenConnects) { // H 왼쪽 위 누락은 원문 gSid 진단 뒤 공통 연결을 계속한다.
    SurfaceCurrentScene s;s.state.ownerPropagationSuppressed=false;Put(s.Slot(50),36,8);std::vector<std::string> events;IslandPostPopHooks hooks;hooks.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="gSid" && file==R"(\Ns\O\Construction.cpp)" && line==0x10a);events.emplace_back(expression); };hooks.connect=[&](Sid) { events.emplace_back("connect"); };auto body=s.Body(hooks);body.Prefix(Sid{50},1);CHECK(events==std::vector<std::string>({"gSid","connect"}));
}
TEST_CASE(IslandCurrent_SupportDetailUsesLiveMapTypeAndCurrentSignedTypeWas) { // 각 소유자/표시 뒤 현재 TYPE와 다음 칸의 현재 머리 및 최종 문맥을 문자열로 전달한다.
    SurfaceCurrentScene s;s.fixed.assign(65536,0);s.fixed[28*256+28]=60;s.Slot(60)[10]=93;s.Slot(61)[10]=93;std::vector<std::string> events;std::string detail;std::uint32_t typeWas=7;IslandPostPopHooks hooks;
    hooks.objects.setOwner=[&](Sid sid,auto owner) { CHECK(owner==5);events.push_back("owner"+std::to_string(sid.value)); };
    hooks.updateDisplay=[&](Sid sid) { events.push_back("display"+std::to_string(sid.value));if (sid==Sid{60}) { s.Slot(60)[10]=74;s.fixed[29*256+28]=61;s.links.islandType=94; }else s.links.stalagType=95; };
    hooks.findTypeAt=[&](float x,float y,auto type) { CHECK(x==30 && y==30 && (type==94 || type==95));events.push_back("find"+std::to_string(type));typeWas=0xffffffffU;return Sid{}; };hooks.typeWas=[&] { return typeWas; };
    hooks.detail=[&](auto text) { detail=text;events.emplace_back("detail"); };hooks.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="0" && file==R"(\Ns\O\Damageable.cpp)" && line==0x18b);CHECK(detail=="[74]X------- (No islandSid)  (No islandStalagSid)  TypeWas -1");events.emplace_back("assert"); };auto body=s.Body(hooks);body.SetSupportOwner(30,30,5);CHECK(events==std::vector<std::string>({"owner60","display60","owner61","display61","find94","find95","detail","assert"}));
}
TEST_CASE(IslandCurrent_SupportEffectsSurviveMissingLateDiagnosis) { // 최종 진단 서비스 누락 앞에 정상 칸의 소유자/표시/두 조회는 이미 실행된다.
    SurfaceCurrentScene s;std::uint32_t owners{},updates{},finds{};IslandPostPopHooks hooks;hooks.objects.setOwner=[&](auto,auto) { ++owners; };hooks.updateDisplay=[&](auto) { ++updates; };hooks.findTypeAt=[&](auto,auto,auto) { ++finds;return Sid{}; };auto body=s.Body(hooks);CHECK(Throws([&] { body.SetSupportOwner(30,30,4); }) && owners==1 && updates==1 && finds==2);
}
TEST_CASE(IslandCurrent_TypeWasOwnerSurvivesGeneralWorldReset) { // 같은 장기 진단 문맥은 일반 월드 준비로 새 BSS0을 만들지 않는다.
    RawGameAuxState owner;CHECK(owner.SupportTypeWas()==0);auto* address=&owner.SupportTypeWas();owner.SupportTypeWas()=0xffffffffU;owner.ResetForGeneralWorld();const auto& current=owner;CHECK(&current.SupportTypeWas()==address && current.SupportTypeWas()==0xffffffffU);
}
