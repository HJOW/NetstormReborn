// 실제 사정거리의 필드512바이트·raw102슬롯·전역·모든 효과와 연속 호출을 대조한다.
#include "RawSceneSupport.h"
#include "client/RawCursorRange.h"
#include <cstring>
using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 원문 DWORD를 입력 순서대로 읽는다. 기대값은 재계산하지 않는다.
std::vector<std::uint32_t> Numbers(std::string_view text) { std::vector<std::uint32_t> values;for (const auto& item:Split(text,',')) values.push_back(Number(item));return values; }
// 명시 입력의 두 DWORD를 double로 읽는다.
double InputTime(const std::vector<std::uint32_t>& input,std::size_t offset) { double value{};std::memcpy(&value,input.data()+offset,8);return value; }
// 몸체가 사용하는 것과 같은 현재 Pool/TYPE/총 자료다. 외부 효과만 독립 입력으로 공급한다.
struct RangeScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false};SquidHash hash; // 실제 단일 레이아웃과 현재 총 검색 공간이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 부모 TYPE/그룹/핫스폿이다.
    std::vector<GunTargetType> gunTypes=std::vector<GunTargetType>(188); // 현재 사거리/테마/방향 프레임이다.
    std::vector<SquidDisplayShape> shapes=std::vector<SquidDisplayShape>(188); // 실제 총 모듈의 타입 번호 계약이다.
    GunTargetState target;CursorManaGlobals coordinates;CursorRangeGlobals globals{coordinates,target}; // 픽셀/Wind/맵은 같은 현재 저장소다.
    SquidProcessState clock;CursorDropState cursor;CursorRangeFields fields; // 독립 입력을 주는 실제 전역·필드다.
    std::vector<std::uint32_t> input;std::string trace,steps; // 저장 입력58열과 실제 호출·단계 결과다.
    std::uint32_t created{},returned{};bool mutated{},killed{}; // 외부 할당/콜백 입력의 현재 상태다.
    std::unique_ptr<RawGunTarget> gun; // 방향/발자국을 검사 전용 식으로 대신하지 않는 실제 모듈이다.
    // fixture에서 쓰는 모든 슬롯을 실제 Pool에서 확보한다.
    RangeScene() { for (std::uint16_t sid=5;sid<=300;++sid) CHECK(pool.Allocate(2)==Sid{sid});gun=std::make_unique<RawGunTarget>(pool,hash,types,gunTypes,shapes,target); }
    // free/dead 입력도 준비하는 검사 전용 접근이며 제품의 할당 검사를 바꾸지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 모든 DWORD/float 인수와 호출 순서를 원문 형식으로 기록한다.
    void Event(std::string_view kind,std::initializer_list<std::uint32_t> values={}) { if (!trace.empty()) trace+=';';trace+=kind;for (const auto value:values) trace+=':'+std::to_string(value); }
    // 원본 실행기에 공급했던 명시 외부 효과 입력만 상태를 바꾼다.
    void Mutate(std::string_view kind) {
        if (mutated) return;const auto mode=input[19];
        if (!((mode==1 && kind=="D") || (mode==2 && kind=="V") || (mode==3 && kind=="X") || (mode==4 && kind=="D") || (mode==5 && kind=="O") || (mode==6 && kind=="A"))) return;
        mutated=true;if (mode>=1 && mode<=5) { Put(fields.bytes,0x1a4,1);Put(fields.bytes,0x1a8,1); }if (mode==4) Put(fields.bytes,0x1e8,0);
        clock.now+=mode==1 ? .125 : mode==2 ? .25 : mode==5 ? .5 : mode==6 ? 1.25 : 0;
    }
    // 이전 행의 결과를 넘기지 않고 입력 바이트만 준비한다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        CHECK(values.size()==58);input=values;trace.clear();steps.clear();created=returned=0;mutated=killed=false;fields.bytes.fill(0xa5);cursor={};cursor.cursorGump=values[16];
        coordinates.mapLimit=std::bit_cast<std::int32_t>(values[18]);coordinates.invalidPoint=std::bit_cast<float>(values[35]);target.scaleX=std::bit_cast<float>(values[47]);target.scaleY=std::bit_cast<float>(values[48]);target.windHalfAngle=std::bit_cast<float>(values[53]);clock.now=InputTime(values,22);
        // TYPE/총 값은 입력이며 getter/판정은 실제 모듈이 실행한다.
        for (const std::uint32_t number:{100U,101U,128U,140U}) {
            types[number].group=std::bit_cast<std::int32_t>(values[1]);types[number].theme=std::bit_cast<std::int32_t>(values[2]);types[number].flags2=values[3];types[number].footX=std::bit_cast<std::int32_t>(values[51]);types[number].footY=std::bit_cast<std::int32_t>(values[52]);types[number].hotspotX=std::bit_cast<std::int32_t>(values[45]);types[number].hotspotY=std::bit_cast<std::int32_t>(values[46]);
            gunTypes[number].range=std::bit_cast<float>(number==values[54] ? values[57] : values[5]);gunTypes[number].theme=types[number].theme;gunTypes[number].directionFrames={0,1,2,3};
        }
        // 원문의 raw102개 전체 표식을 유지하며 TYPE/상태/좌표의 입력만 바꾼다.
        for (std::uint16_t sid=50;sid<=251;++sid) {
            if (sid>=150 && sid<250) continue;auto bytes=Raw(Sid{sid});std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa5});Put(bytes,0,1);bytes[10]=sid==250 ? static_cast<std::uint8_t>(values[4]) : sid==251 ? 6 : 104;bytes[11]=sid>=250 ? 0 : static_cast<std::uint8_t>(values[13]);bytes[34]=1;bytes[40]=sid==250 ? static_cast<std::uint8_t>(values[17]) : 0;Put(bytes,6,sid==250 ? 300 : 0,2);Put(bytes,14,values[sid>=250 ? 28 : 30]);Put(bytes,18,values[sid>=250 ? 29 : 31]);Put(bytes,36,2);
        }
        Put(Raw(Sid{251}),18,values[54],2);Put(fields.bytes,0,0x50c498);Put(fields.bytes,4,300);Put(fields.bytes,8,250);Put(fields.bytes,12,values[36]);Put(fields.bytes,16,values[1]==6 ? 1 : 0);
        // SID0의 첫 칸/전체 칸 입력도 원문과 같은100칸 배열에 준비한다.
        for (std::size_t index=0;index<100;++index) Put(fields.bytes,20+4*index,(values[14]==2 || (values[14]==1 && index==0)) ? 0 : static_cast<std::uint32_t>(50+index));
        const std::array<std::pair<std::size_t,std::uint32_t>,13> initial{{{0x1a4,values[37]},{0x1a8,values[8]},{0x1ac,values[37]},{0x1b0,values[34]},{0x1b4,values[7]},{0x1b8,values[9]},{0x1bc,std::bit_cast<std::uint32_t>(5.0f)},{0x1c0,values[39]},{0x1c4,values[40]},{0x1c8,values[41]},{0x1cc,values[42]},{0x1e8,values[10]},{0x1ec,values[38]}}}; // 명시 필드 오프셋과 입력의 대응이다.
        // 입력 필드만 바꾸며 다른 padding은 표식 그대로 둔다.
        for (const auto& [offset,value]:initial) Put(fields.bytes,offset,value);
        const auto previous=InputTime(values,24),createdTime=InputTime(values,55),deadline=InputTime(values,26);std::memcpy(fields.bytes.data()+0x1d0,&previous,8);std::memcpy(fields.bytes.data()+0x1d8,&createdTime,8);std::memcpy(fields.bytes.data()+0x1e0,&deadline,8);
        // 네 방향의 허용은 frame 단독 실행의 명시 입력이다. 생성자는 실제 판정을 실행한다.
        for (std::size_t side=0;side<4;++side) Put(fields.bytes,0x1f0+4*side,(values[15]&(1U<<side)) ? 1 : 0);
    }
    // 계산 대신 실제 경계 인수/효과와 TYPE 자료만 공급한다.
    CursorRangeHooks Hooks() {
        CursorRangeHooks hooks;
        hooks.world.create=[this](std::uint32_t type,std::uint32_t flags) { const auto sid=static_cast<std::uint16_t>(50+created++);Event("C",{type,flags,sid});return Sid{sid}; };
        hooks.world.detach=[this](Sid sid,std::uint32_t flags) { Event("D",{sid.value,flags});Raw(sid)[11]|=4;Mutate("D"); };
        hooks.world.unpop=[this](Sid sid,std::uint32_t flags) { Event("U",{sid.value,flags});Raw(sid)[11]&=~4;Mutate("U"); };
        hooks.world.destroy=[this](Sid sid,std::uint32_t flags) { Event("X",{sid.value,flags});Raw(sid)[11]|=2;Mutate("X"); };
        hooks.world.position=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("V",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags});Put(Raw(sid),14,std::bit_cast<std::uint32_t>(x));Put(Raw(sid),18,std::bit_cast<std::uint32_t>(y));Raw(sid)[11]&=~4;Mutate("V"); };
        hooks.world.center=[this](Sid sid) { Event("O",{sid.value});Mutate("O");return std::array{std::bit_cast<float>(input[32]),std::bit_cast<float>(input[33])}; };
        hooks.range=[this](std::uint32_t type) { return gunTypes.at(type).range; };
        hooks.firstContent=[this](Sid sid) { Event("T",{sid.value,1,0,0});return Sid{251}; };
        hooks.mode=[this](Sid sid) { Event("M",{sid.value});return input[6]; };
        hooks.footprint=[this](Sid sid) { Event("P",{sid.value});return gun->Footprint(sid,false); };
        hooks.cannonDirection=[this](const GunTargetQuery& q,std::int32_t direction) { Event("B",{std::bit_cast<std::uint32_t>(q.x),std::bit_cast<std::uint32_t>(q.y),std::bit_cast<std::uint32_t>(q.centerX),std::bit_cast<std::uint32_t>(q.centerY),q.type,std::bit_cast<std::uint32_t>(q.direction),q.owner,std::bit_cast<std::uint32_t>(q.frame),std::bit_cast<std::uint32_t>(direction)});return gun->CannonDirection(q,direction); };
        hooks.directionFrame=[this](Sid sid,std::int32_t direction) { Event("F",{sid.value,std::bit_cast<std::uint32_t>(direction)});return gun->DirectionFrame(Raw(sid)[10],direction); };
        hooks.firingOffset=[this](std::uint32_t type,std::int32_t frame) { Event("H",{type,0,std::bit_cast<std::uint32_t>(frame)});return std::array{std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(input[49])),std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(input[50]))}; };
        hooks.frameRun=[this](std::uint32_t type,std::uint32_t variant) { CHECK(type==104 && variant<2);return RiftTypeFrames::LetterRun{static_cast<int>(input[43]+variant*10),static_cast<int>(input[44]+variant)}; };
        hooks.diagnostic=[this](std::uint32_t color,std::string_view text) { CHECK(text=="Dead circle range!");Event("Q",{color}); };return hooks;
    }
    // 실제 Host 부착의 필드 쓰기와 명시 현재 시각 변경을 공급한다.
    CursorRangeAttach Attach() { return [this](CursorRangeFields& actual,Sid parent,Sid form,std::uint32_t flags) { CHECK(&actual==&fields);Event("A",{51,parent.value,form.value,flags});Put(fields.bytes,4,form.value ? form.value : 300);Put(fields.bytes,8,parent.value);Mutate("A"); }; }
    // 한 실제 호출 후 필드512바이트를 모두 기록한다.
    void Snapshot() { if (!steps.empty()) steps+='|';steps+=Hex(fields.bytes); }
    // 원문과 같은 단일/연속 호출을 실행한다.
    void Run(RawCursorRange& logic) {
        const auto frame=[&] { logic.Frame(fields,[&] { Event("K",{0});killed=true;logic.Destroy(fields,1); });Snapshot(); };
        if (input[0]==0 || input[0]==5) {
            logic.Construct(fields,Sid{250},{},Attach());Snapshot();
            if (input[0]==5) {
                // 생성 필드를 그대로 이어 정상 프레임을 실행한다.
                for (std::uint32_t index=0;index<input[21];++index) { clock.now+=.125;frame(); }
                logic.Notify(fields,3,0,0);Snapshot();
                // 실제 종료의 즉시/반경 축소/다음 프레임 자기 삭제를 이어 관찰한다.
                for (std::uint32_t index=0;index<3 && !killed;++index) { clock.now+=.25;frame(); }
                if (!killed) { logic.Destroy(fields,1);Snapshot(); }
            }
        } else if (input[0]==1) frame();
        else if (input[0]==2) { logic.Notify(fields,input[11],input[12],0x87654321);Snapshot(); }
        else if (input[0]==3) { logic.Destroy(fields,1);Snapshot(); }
        else if (input[0]==4) { Event("N",{0x200});if (input[20]) { logic.Construct(fields,Sid{250},{},Attach());returned=1; }Snapshot(); }
    }
    // 가상 표 토큰을 포함한 전체102개 raw 슬롯의 결과다.
    std::string RawSnapshot() { std::string result;for (std::uint16_t sid=50;sid<=251;++sid) if (sid<150 || sid>=250) result+=Hex(pool.Slot(Sid{sid}));return result; }
    // 현재 커서 토큰·double 시각·할당 반환값이다.
    std::string Globals() { const auto words=std::bit_cast<std::array<std::uint32_t,2>>(clock.now);return std::to_string(cursor.cursorGump)+","+std::to_string(words[0])+","+std::to_string(words[1])+","+std::to_string(returned); }
};
}
// 두 실제 원문 중 기본53비트 전체 바이트·전역·효과를 C++와 대조한다.
TEST_CASE(CursorRange_NativeBodiesAndFullState) {
    RangeScene scene;std::size_t count{},different{};
    // 기대값을 계산하지 않고 독립 원문을 재생한다.
    for (const auto& row:LoadFixture(NETSTORM_CURSOR_RANGE_FIXTURE).rows) {
        CHECK(row.size()==11);scene.Prepare(Numbers(row[0]));RawCursorRange logic(scene.pool,scene.types,scene.clock,scene.globals,scene.cursor,scene.Hooks());scene.Run(logic);
        const auto trace=scene.trace.empty() ? "-" : scene.trace;const auto fields=Hex(scene.fields.bytes);const auto globals=scene.Globals();const auto raw=scene.RawSnapshot();const bool same=trace==row[1] && fields==row[2] && globals==row[3] && raw==row[4] && scene.steps==row[5];CHECK(same);
        if (!same) { std::printf("사정거리 행%zu 입력%s\n기대%s\n실제%s\n필드%s\n%s\n전역%s/%s\n",count,row[0].c_str(),row[1].c_str(),trace.c_str(),row[2].c_str(),fields.c_str(),row[3].c_str(),globals.c_str());break; }
        if (row[6]!="-") ++different;++count;
    }
    CHECK(count==593 && different==0);
}
// 실제 공유 전역과 잘못된 판본/타입/목록/효과 계약을 확인한다.
TEST_CASE(CursorRange_SharedStateAndGuards) {
    RangeScene scene;scene.Prepare(Numbers(LoadFixture(NETSTORM_CURSOR_RANGE_FIXTURE).rows.front()[0]));RawCursorRange logic(scene.pool,scene.types,scene.clock,scene.globals,scene.cursor,scene.Hooks());
    CHECK(&logic.Pool()==&scene.pool && logic.ProcessType()==51 && &scene.globals.coordinates==&scene.coordinates && &scene.globals.targeting==&scene.target);
    SidPool cd(OriginalEdition::Cd1072,24000,false);CHECK(Throws([&] { RawCursorRange invalid(cd,scene.types,scene.clock,scene.globals,scene.cursor,scene.Hooks()); }));
    CHECK(Throws([&] { RawCursorRange invalid(scene.pool,std::span(scene.types).first(187),scene.clock,scene.globals,scene.cursor,scene.Hooks()); }));
    RawCursorRange missing(scene.pool,scene.types,scene.clock,scene.globals,scene.cursor,{});CHECK(Throws([&] { missing.Construct(scene.fields,Sid{250},{},scene.Attach()); }));
    Put(scene.fields.bytes,20,24000);CHECK(Throws([&] { logic.Destroy(scene.fields,1); }));
}
