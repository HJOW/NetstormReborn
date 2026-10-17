// 실제 마나 프로세스의 전체 필드·raw31슬롯·전역·모든 효과와 연속 호출을 대조한다.
#include "RawSceneSupport.h"
#include "client/RawCursorMana.h"
#include "o/SquidDisplay.h"
#include "client/Renderer.h"
#include <cstring>
using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// DWORD 입력을 원문 순서대로 읽으며 기대값은 계산하지 않는다.
std::vector<std::uint32_t> Numbers(std::string_view value) { std::vector<std::uint32_t> result;for (const auto& item:Split(value,',')) result.push_back(Number(item));return result; }
// 입력의 두 DWORD를 원본 double 시각으로 읽는다.
double InputTime(const std::vector<std::uint32_t>& input,std::size_t index) { double result{};std::memcpy(&result,input.data()+index,8);return result; }
// 독립 가상 경계만 공급하며 생성/이동/이벤트/삭제의 분기는 실제 C++ 몸체가 계산한다.
struct ManaScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 부모·표시 객체가 공유하는 실제 레이아웃이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 TYPE 입력이다.
    SquidProcessState clock; // Host와 같은 double 시각의 독립 입력이다.
    CursorManaGlobals globals;CursorDropState cursor;CursorManaFields fields; // 실제 단일 전역·필드 저장소다.
    std::vector<std::uint32_t> input; // 저장된36DWORD 입력이다.
    std::string trace,steps; // 모든 가상 효과와 각 실제 호출 후 전체 필드다.
    std::uint32_t created{},returned{};bool mutated{}; // 외부 할당/콜백 입력의 현재 상태다.
    // raw 입력 슬롯을 실제 Pool에서 확보한다. 테스트는 게임/장치를 열지 않는다.
    ManaScene() { for (std::uint16_t value=5;value<=150;++value) CHECK(pool.Allocate(2)==Sid{value}); }
    // free/dead 입력도 준비하는 검사 전용 접근이다. 제품은 이 접근으로 할당 검사를 우회하지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 원문의 가상 경계 사건 형식을 유지한다.
    void Event(std::string_view kind,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=kind;
        // signed/float 인자도 전체 DWORD로 기록한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 최초 선택된 외부 효과만 현재 개수/시간을 바꾸는 명시 입력이다.
    void Mutate(std::string_view kind) {
        if (mutated) return;const auto mode=input[19];
        if (!((mode==1 && kind=="D") || (mode==2 && kind=="V") || (mode==3 && kind=="X") || (mode==4 && kind=="D") || (mode==5 && kind=="O") || (mode==6 && kind=="A"))) return;
        mutated=true;if (mode>=1 && mode<=5) Put(fields.bytes,0x84,1);if (mode==4) Put(fields.bytes,0xd8,0);
        clock.now+=mode==1 ? .125 : mode==2 ? .25 : mode==5 ? .5 : mode==6 ? 1.25 : 0;
    }
    // 실행 전 입력만 준비하며 이전 행의 결과는 공유하지 않는다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        CHECK(values.size()==36);input=values;trace.clear();steps.clear();created=returned=0;mutated=false;fields.bytes.fill(0xa5);
        cursor={};cursor.cursorGump=values[17];globals.mapLimit=std::bit_cast<std::int32_t>(values[18]);globals.radiusLimit=std::bit_cast<float>(values[34]);globals.invalidPoint=std::bit_cast<float>(values[35]);clock.now=InputTime(values,22);
        types[100].theme=std::bit_cast<std::int32_t>(values[1]);types[100].footX=std::bit_cast<std::int32_t>(values[2]);types[100].footY=std::bit_cast<std::int32_t>(values[3]);
        // raw 슬롯의 무관한 바이트도 표식으로 채워 실제 쓰기 범위를 검사한다.
        for (std::uint16_t sid=50;sid<=150;++sid) {
            if (sid>=80 && sid!=150) continue;auto bytes=Raw(Sid{sid});std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa5});Put(bytes,0,1);bytes[10]=sid==150 ? 100 : 103;
            bytes[11]=sid==150 ? 0 : static_cast<std::uint8_t>(values[13]|((values[15]>>(sid-50)&1) ? 4 : 0)|((values[16]>>(sid-50)&1) ? 2 : 0));Put(bytes,6,sid==150 ? 90 : 0,2);
            Put(bytes,14,values[sid==150 ? 28 : 30]);Put(bytes,18,values[sid==150 ? 29 : 31]);
        }
        Put(fields.bytes,0,0x50c468);Put(fields.bytes,4,90);Put(fields.bytes,8,150);
        // SID0은 명시 입력이며 생성/판정의 기대값을 만들지 않는다.
        for (std::size_t index=0;index<30;++index) Put(fields.bytes,12+4*index,(values[14]>>index&1) ? 0 : static_cast<std::uint32_t>(50+index));
        Put(fields.bytes,0x84,values[4]);Put(fields.bytes,0x88,values[5]);Put(fields.bytes,0x98,values[6]);Put(fields.bytes,0x90,values[7]);Put(fields.bytes,0x8c,values[8]);Put(fields.bytes,0x94,values[9]);Put(fields.bytes,0xd8,values[10]);
        // 사각 위상은 fixture 생성기의 명시 입력이며 출력 보간을 대체하지 않는다.
        for (std::size_t index=0;index<7;++index) Put(fields.bytes,0x9c+4*index,std::bit_cast<std::uint32_t>(.25f*static_cast<float>(index+1)));
        Put(fields.bytes,0xbc,std::bit_cast<std::uint32_t>(static_cast<float>(types[100].footX-1)));Put(fields.bytes,0xb8,std::bit_cast<std::uint32_t>(static_cast<float>(types[100].footY-1)));
        const auto previous=InputTime(values,24),createdAt=3.25,deadline=InputTime(values,26);std::memcpy(fields.bytes.data()+0xc0,&previous,8);std::memcpy(fields.bytes.data()+0xc8,&createdAt,8);std::memcpy(fields.bytes.data()+0xd0,&deadline,8);
    }
    // 실제 분기/산술 대신 외부 효과의 입력/출력과 현재 상태 변경만 제공한다.
    CursorManaHooks Hooks() {
        CursorManaHooks hooks;
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { const auto sid=static_cast<std::uint16_t>(50+created++);Event("C",{type,flags,sid});return Sid{sid}; };
        hooks.setWord=[this](Sid sid,std::uint32_t value) { Put(Raw(sid),12,value,2); };hooks.themeFrame=CursorManaThemeFrame;
        hooks.detach=[this](Sid sid,std::uint32_t flags) { Event("D",{sid.value,flags});Raw(sid)[11]|=4;Mutate("D"); };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("U",{sid.value,flags});Raw(sid)[11]&=~4;Mutate("U"); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event("X",{sid.value,flags});Raw(sid)[11]|=2;Mutate("X"); };
        hooks.position=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("V",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags});Put(Raw(sid),14,std::bit_cast<std::uint32_t>(x));Put(Raw(sid),18,std::bit_cast<std::uint32_t>(y));Raw(sid)[11]&=~4;Mutate("V"); };
        hooks.center=[this](Sid sid) { Event("O",{sid.value});Mutate("O");return std::array{std::bit_cast<float>(input[32]),std::bit_cast<float>(input[33])}; };return hooks;
    }
    // 실제 Attach의 Form/부모 쓰기와 외부 현재 시각 변경을 제공한다.
    CursorManaAttach Attach() { return [this](CursorManaFields& actual,Sid parent,Sid form,std::uint32_t flags) { CHECK(&actual==&fields);Event("A",{50,parent.value,form.value,flags});Put(fields.bytes,4,form.value ? form.value : 90);Put(fields.bytes,8,parent.value);Mutate("A"); }; }
    // 한 실제 함수 호출 뒤224바이트 전부를 보존한다.
    void Snapshot() { if (!steps.empty()) steps+='|';steps+=Hex(fields.bytes); }
    // 같은 실제 몸체에 저장 입력의 단일/연속 호출을 재생한다.
    void Run(RawCursorMana& logic) {
        const auto frame=[&] { logic.Frame(fields,[&] { Event("K",{0});logic.Destroy(fields,1); });Snapshot(); };
        if (input[0]==0 || input[0]==5) {
            logic.Construct(fields,Sid{150},{},Attach());Snapshot();
            if (input[0]==5) {
                // 생성 결과를 그대로 이어 실제 여러 프레임과 종료/소멸을 실행한다.
                for (std::uint32_t index=0;index<input[21];++index) { clock.now+=.125;frame(); }
                logic.Notify(fields,3,0,0);Snapshot();logic.Destroy(fields,1);Snapshot();
            }
        } else if (input[0]==1) frame();
        else if (input[0]==2) { logic.Notify(fields,input[11],input[12],0x87654321);Snapshot(); }
        else if (input[0]==3) { logic.Destroy(fields,1);Snapshot(); }
        else if (input[0]==4) { Event("N",{0xe0});if (input[20]) { logic.Construct(fields,Sid{150},{},Attach());returned=1; }Snapshot(); }
    }
    // 전체 현재 raw31슬롯을 원문과 같은 순서로 읽는다.
    std::string RawSnapshot() { std::string result;for (std::uint16_t sid=50;sid<=150;++sid) if (sid<80 || sid==150) result+=Hex(pool.Slot(Sid{sid}));return result; }
    // 현재 커서 토큰·double 시각·new 반환 식별값이다.
    std::string Globals() { const auto words=std::bit_cast<std::array<std::uint32_t,2>>(clock.now);return std::to_string(cursor.cursorGump)+","+std::to_string(words[0])+","+std::to_string(words[1])+","+std::to_string(returned); }
};
}
// 기본53비트와64비트 원문이 모두 같았던642입력의 전체 상태와 효과를 대조한다.
TEST_CASE(CursorMana_NativeConstructorFrameNotifyDestructor642) {
    ManaScene scene;std::size_t count{};
    // 실제 PE 결과를 읽으며 기대값을 다시 계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_CURSOR_MANA_FIXTURE).rows) {
        CHECK(row.size()==11);scene.Prepare(Numbers(row[0]));RawCursorMana logic(scene.pool,scene.types,scene.clock,scene.globals,scene.cursor,scene.Hooks());scene.Run(logic);
        const auto trace=scene.trace.empty() ? "-" : scene.trace;const auto fields=Hex(scene.fields.bytes);const auto globals=scene.Globals();const auto raw=scene.RawSnapshot();
        const bool same=trace==row[1] && fields==row[2] && globals==row[3] && raw==row[4] && scene.steps==row[5];CHECK(same);
        if (!same) { std::printf("마나 행%zu 입력%s\n기대%s\n실제%s\n필드%s\n%s\n전역%s/%s\n",count,row[0].c_str(),row[1].c_str(),trace.c_str(),row[2].c_str(),fields.c_str(),row[3].c_str(),globals.c_str());break; }
        CHECK(row[1]==row[6] && row[2]==row[7] && row[3]==row[8] && row[4]==row[9] && row[5]==row[10]);++count;
    }
    CHECK(count==642);
}
// 미복원 경계를 생략하지 않으며 잘못된 판본/타입과 목록/테마 범위를 거부한다.
TEST_CASE(CursorMana_ContractsAndCurrentFieldBounds) {
    ManaScene scene;scene.Prepare(Numbers(LoadFixture(NETSTORM_CURSOR_MANA_FIXTURE).rows.front()[0]));RawCursorMana logic(scene.pool,scene.types,scene.clock,scene.globals,scene.cursor,scene.Hooks());
    CHECK(&logic.Pool()==&scene.pool && logic.ProcessType()==50);CHECK(Throws([&] { CursorManaThemeFrame(-1); }));CHECK(Throws([&] { CursorManaThemeFrame(11); }));
    SidPool cd(OriginalEdition::Cd1072,24000,false);CHECK(Throws([&] { RawCursorMana invalid(cd,scene.types,scene.clock,scene.globals,scene.cursor,scene.Hooks()); }));
    CHECK(Throws([&] { RawCursorMana invalid(scene.pool,std::span(scene.types).first(187),scene.clock,scene.globals,scene.cursor,scene.Hooks()); }));
    RawCursorMana missing(scene.pool,scene.types,scene.clock,scene.globals,scene.cursor,{});CHECK(Throws([&] { missing.Construct(scene.fields,Sid{150},{},scene.Attach()); }));
    Put(scene.fields.bytes,8,150);Put(scene.fields.bytes,0x98,0);Put(scene.fields.bytes,0xd8,1);Put(scene.fields.bytes,0x94,0);Put(scene.fields.bytes,0x84,31);
    CHECK(Throws([&] { logic.Destroy(scene.fields,1); }));
}

namespace {
// 실제 Renderer의 변경표와 외부 전체 억제를 구별한다.
struct ManaDisplaySink final:SquidDisplaySink {
    DirtyRegions dirty{640,480}; // 원본과 같은 사각형 병합/표식 확장이다.
    bool suppressed{}; // 원본0059a8b0의 별도 전체 표시 억제 입력이다.
    bool Suppressed() const override { return suppressed || dirty.FullRedraw(); } // 실제 helper가 호출 직전에 조회한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t flags) override { dirty.Add({rect.left,rect.top,rect.right,rect.bottom},flags); } // 실제 변경표에 전달한다.
};
}
// 전달한 마나103 또는 볼트105 fixture의432입력을 실제 bounds/clip/dirty 전체 변경표와 대조한다.
void ReplayNativeManaDisplay(const char* fixture) {
    std::vector<RiftTypeRecord> types(188);std::vector<SquidDisplayShape> shapes(188); // 원문의 합성 TYPE/SHP 입력이다.
    shapes[74].frameCount=2;shapes[74].loaded=true;shapes[74].frames={{100,120,-3,5,1,1},{130,140,7,-9,1,1},{50,70,2,3,1,1},{80,90,4,6,1,1}};
    types[74].flags2=0x10000;types[74].footX=types[74].footY=1;ManaDisplaySink sink;SquidDisplay display(OriginalEdition::Patch1078,types,shapes,sink,{7,9,65536,{12,9,620,450}});
    std::size_t count{}; // 실제 고유 원문 입력 수다.
    // 입력 raw 전체를 그대로 사용하고 출력 사각형을 테스트에서 계산하지 않는다.
    for (const auto& row:LoadFixture(fixture).rows) {
        CHECK(row.size()==4);const auto input=Numbers(row[0]);CHECK(input.size()==7);sink.dirty.Clear();sink.suppressed=input[5]!=0;types[74].flags1=input[2];display.SetView({7,9,static_cast<int>(input[3]),{12,9,620,450}});
        std::vector<std::uint8_t> bytes(50); // 원본 호출 직전 입력 슬롯이다.
        // WORD 옆 바이트/플래그/좌표도 원문에 기록된 바이트를 그대로 읽는다.
        for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(row[1].substr(index*2,2),nullptr,16));
        const auto before=bytes;display.Update(bytes,input[6] ? 0x2000 : 0);CHECK(bytes==before);std::string output;
        // 실제 dirty 변경표의 순서와 각 flags를 모두 보존한다.
        for (const auto& entry:sink.dirty.Entries()) { if (!output.empty()) output+=';';const auto r=entry.rect;output+=std::to_string(r.left)+","+std::to_string(r.top)+","+std::to_string(r.right)+","+std::to_string(r.bottom)+","+std::to_string(entry.flags); }
        if (output.empty()) output="-";CHECK(row[2]==std::to_string(sink.suppressed ? 1 : 0));CHECK(output==row[3]);++count;
    }
    CHECK(count==432);
}

// 실제473770/8c의 WORD 조건과10픽셀 확대를 독립432입력으로 검증한다.
TEST_CASE(CursorMana_NativeDirtyDisplay432) { ReplayNativeManaDisplay(NETSTORM_CURSOR_MANA_DISPLAY_FIXTURE); }
// 실제45ac90/8c의 WORD 무관/10픽셀 확대를 별도432입력으로 검증한다.
TEST_CASE(ManaBolt_NativeDirtyDisplay432) { ReplayNativeManaDisplay(NETSTORM_MANA_BOLT_DISPLAY_FIXTURE); }
