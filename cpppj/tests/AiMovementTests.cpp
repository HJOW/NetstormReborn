// AI 이동의 실제 기하/원문과 권한·경로·이름 조회 순서를 검사한다. 작성·컴파일만 한다.
#include "o/RawAiMovement.h"
#include "RawSceneSupport.h"
#include <cmath>
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 실제 raw 풀/본체와 명시 경로·전송·이름 장치를 구성한다.
struct AiMoveScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true};RawAiBrainMemory memory{0x4a000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //실제부모/대상/본체의소유저장소다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)};Sid actor{},target{},other{}; //현재본체와실제세슬롯이다.
    bool boss{true};std::uintptr_t path{1};double distance{2};CarrierMoveState packet;BoardTargetState boardState;RawBoardTarget board{boardState}; //현재권한/명시경로반환/거리/같은헤더·기하다.
    AiMovementHooks hooks;std::vector<std::string> trace;std::vector<std::vector<std::uint8_t>> packets;RawPathStartRequest started; //선택효과/실제순서/원문과경로인자다.
    Sid moved{};std::int32_t logX{},logY{};std::string sourceName,targetName; //실제대상/절삭좌표/이름로그인자다.
    // 원본 입력과 관찰할 하위만 준비한다.
    AiMoveScene() {
        actor=New();target=New();other=New();auto raw=pool.AllocatedBytes(actor);raw[10]=158;Put(raw,14,std::bit_cast<std::uint32_t>(12.0F));Put(raw,18,std::bit_cast<std::uint32_t>(5.0F));Field(0x260,2);Field(0xb00,1);Field(0x294,target.value);
        hooks.movement=[this](auto type) { CHECK(type==158);trace.push_back("movement");return 54U; };hooks.lookup=[this](Sid parent,auto type,auto index,bool remove) { CHECK(parent==actor && type==54 && index==0);trace.push_back(remove ? "remove" : "lookup");return Sid{50}; };
        hooks.footprint=[this](Sid sid) { CHECK(sid==target);trace.push_back("footprint");return SquidFootprint{10,10,14,14}; };hooks.send=[this](auto channel,std::span<const std::uint8_t> bytes) { CHECK(channel==9);trace.push_back("send");packets.emplace_back(bytes.begin(),bytes.end()); };
        hooks.sacrifice=[this](Sid parent,Sid sid) { CHECK(parent==actor && sid==target);trace.push_back("sacrifice"); };hooks.target=[this](Sid parent,Sid sid,auto replace,auto helper) { CHECK(parent==actor && !replace && !helper);moved=sid;trace.push_back("target");return path; };hooks.start=[this](RawPathStartRequest request) { started=request;trace.push_back("start");return path; };
        hooks.name=[this](Sid sid) { trace.push_back("name:"+std::to_string(sid.value));return sid==actor ? std::string("actor") : "target"+std::to_string(sid.value); };hooks.logTarget=[this](auto brain,std::string_view source,std::string_view destination) { CHECK(brain==token);sourceName=source;targetName=destination;trace.push_back("logTarget"); };hooks.logPoint=[this](auto brain,std::string_view name,auto x,auto y) { CHECK(brain==token);sourceName=name;logX=x;logY=y;trace.push_back("logPoint"); };hooks.distance=[this](RawPathPoint point,Sid sid) { CHECK(point.x==12 && point.y==5 && sid==target);trace.push_back("distance");return distance; };
    }
    // 같은 실제 슬롯의 payload만 명시0으로 준비한다.
    Sid New() { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});return sid; }
    // 선택된 현재 본체 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 같은 현재 권한/기하/헤더를 빌린다.
    RawAiMovement Make() { return RawAiMovement(memory,pool,boss,board,packet,hooks); }
};
}
// 현재TYPE→순번0/remove를전달하며정지0은하위가없어도무동작이다.
TEST_CASE(AiMovement_CurrentPathLookupAndRemovalArguments) {
    AiMoveScene input;CHECK(input.Make().MovementForm(input.actor)==Sid{50});input.Make().Stop(input.actor);CHECK(input.trace==std::vector<std::string>({"movement","lookup","movement","remove"}));input.hooks={};input.Make().Stop(Sid{});CHECK(input.trace.size()==4);
}
// 서버실패/희생은이름을읽지않고성공은대상→출발이름을디버그조회전에읽는다.
TEST_CASE(AiMovement_ServerTargetBranchesAndNameOrder) {
    AiMoveScene input;input.path=0;input.Make().ToTarget(input.token,input.actor,input.target,0);CHECK(input.trace==std::vector<std::string>({"target"}));input.trace.clear();input.Make().ToTarget(input.token,input.actor,input.target,1);CHECK(input.trace==std::vector<std::string>({"sacrifice"}));input.trace.clear();input.path=1;input.Make().ToTarget(input.token,input.actor,input.target,0);CHECK(input.trace==std::vector<std::string>({"target","name:6","name:5","logTarget"}) && input.sourceName=="actor" && input.targetName=="target6");
    input.trace.clear();input.hooks.name=[&](Sid sid) { input.trace.push_back("name:"+std::to_string(sid.value));input.Field(0xb00,0);return "current"; };input.hooks.logTarget={};input.Make().ToTarget(input.token,input.actor,input.target,0);CHECK(input.trace==std::vector<std::string>({"target","name:6","name:5"}));
}
// 비서버는실제접근점/현재헤더의18BYTE를보내며사용자선택/중복목적지관문은없다.
TEST_CASE(AiMovement_ClientTargetUsesActualBoardAndPacket) {
    AiMoveScene input;input.boss=false;input.packet.request={7,0xf3,2};input.Make().ToTarget(input.token,input.actor,input.target,0xdeadbeefU);CHECK(input.trace==std::vector<std::string>({"footprint","send","name:6","name:5","logTarget"}) && input.packets.size()==1);const auto& bytes=input.packets[0];CHECK(bytes.size()==18 && bytes[0]==0x73 && bytes[1]==7 && Get(bytes,2,2)==2 && Get(bytes,4,2)==input.actor.value && std::bit_cast<float>(Get(bytes,6))==12 && std::bit_cast<float>(Get(bytes,10))==10 && Get(bytes,14)==0xdeadbeefU);
}
// 송신효과뒤권한이바뀌면희생우회없이현재대상이동/좌표경로를시작한다.
TEST_CASE(AiMovement_AuthorityRereadAfterSend) {
    AiMoveScene target;target.boss=false;target.hooks.send=[&](auto channel,auto) { CHECK(channel==9);target.trace.push_back("send");target.boss=true; };target.Make().ToTarget(target.token,target.actor,target.target,1);CHECK(target.trace==std::vector<std::string>({"footprint","send","target","name:6","name:5","logTarget"}));AiMoveScene point;point.boss=false;point.hooks.send=[&](auto,auto) { point.trace.push_back("send");point.boss=true; };point.path=0;point.Make().ToPoint(point.token,point.actor,{4,5},1);CHECK(point.trace==std::vector<std::string>({"send","start"}));
}
// 실제시작인자는나머지0이고로그좌표만원본signed절삭을적용한다.
TEST_CASE(AiMovement_PointStartAndFtolLogging) {
    AiMoveScene input;input.Make().ToPoint(input.token,input.actor,{1.9F,-2.9F},0xdeadbeefU);CHECK(input.started.parent==input.actor && input.started.destination.x==1.9F && input.started.destination.y==-2.9F && !input.started.replaceStart && !input.started.helper && !input.started.tailWord && !input.started.tailByte && !input.started.reserved);CHECK(input.trace==std::vector<std::string>({"start","name:5","logPoint"}) && input.logX==1 && input.logY==-2);input.Make().ToPoint(input.token,input.actor,{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()},0);CHECK(input.logX==0 && input.logY==0);
}
// 거리2이상과unordered는복귀하며거리효과뒤현재신전SID를다시읽는다.
TEST_CASE(AiMovement_HomeBoundaryUnorderedAndCurrentTemple) {
    AiMoveScene input;input.distance=1.999;input.Make().Home(input.token,input.actor);CHECK(input.trace==std::vector<std::string>({"distance"}));input.trace.clear();input.distance=2;input.Make().Home(input.token,input.actor);CHECK(input.moved==input.target && input.trace.front()=="distance");input.trace.clear();input.hooks.distance=[&](RawPathPoint point,Sid temple) { CHECK(point.x==12 && point.y==5 && temple==input.target);input.trace.push_back("distance");input.Field(0x294,input.other.value);Put(input.pool.AllocatedBytes(input.actor),14,std::bit_cast<std::uint32_t>(99.0F));return std::numeric_limits<double>::quiet_NaN(); };input.Make().Home(input.token,input.actor);CHECK(input.moved==input.other && input.trace.front()=="distance");input.trace.clear();input.Field(0x294,0);input.hooks={};input.Make().Home(input.token,input.actor);CHECK(input.trace.empty());
}
// 원본거리의음수내부값/x float저장/unordered x선택을확인한다.
TEST_CASE(AiMovement_FootprintDistanceSignedAndUnordered) {
    const SquidFootprint rectangle{10,10,14,14};CHECK(AiDistanceToFootprint({12,12},rectangle)==-2);CHECK(AiDistanceToFootprint({8,12},rectangle)==2 && AiDistanceToFootprint({12,8},rectangle)==2);const auto nan=std::numeric_limits<float>::quiet_NaN();CHECK(std::isnan(AiDistanceToFootprint({nan,8},rectangle)));CHECK(AiDistanceToFootprint({8,nan},rectangle)==2);
}
// 누락된이름/경로를가짜성공으로바꾸지않고먼저발생한송신/경로효과를보존한다.
TEST_CASE(AiMovement_SelectedMissingEffectsKeepPrefix) {
    AiMoveScene input;input.hooks.name={};CHECK(Throws([&] { input.Make().ToTarget(input.token,input.actor,input.target,0); }) && input.trace==std::vector<std::string>({"target"}));AiMoveScene client;client.boss=false;client.hooks.send=[&](auto,auto bytes) { client.packets.emplace_back(bytes.begin(),bytes.end());client.boss=true; };client.hooks.start={};CHECK(Throws([&] { client.Make().ToPoint(client.token,client.actor,{4,5},0); }) && client.packets.size()==1);
}
