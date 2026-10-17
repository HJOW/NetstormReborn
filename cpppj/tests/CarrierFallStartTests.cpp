// 원본 두 부모를 같은 실제 C++ task/시작 모듈로 재생하며 생성 후 필드 보존도 확인한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierTask.h"
#include "o/RawCarrierFallStart.h"
#include "o/RawPathRuntime.h"
#include <array>
#include <bit>
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원문 입력 DWORD의 부호/NaN 비트를 보존한다.
std::vector<std::uint32_t> StartNumbers(std::string_view text) { std::vector<std::uint32_t> result;
    // 기대값과 분리된 각 입력 칸을 읽는다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// 원문과 같은 큰 외부 효과만 공급하며 내부 task는 실제 RawCarrierTask를 실행한다.
struct CarrierStartScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원문 부모/Form의 물리 풀이다.
    std::vector<std::uint8_t> players=std::vector<std::uint8_t>(128*0xb4); // 쓰지 않은 모든 Player 바이트도 비교한다.
    std::vector<std::uint32_t> row; // 원문20DWORD 입력이다.
    double now{}; // 현재 double 시각이며 외부 효과가 바꿀 수 있다.
    bool boss{}; // 현재 권한이다.
    std::uint32_t priest{}; // 현재 사제 종류다.
    std::unique_ptr<RegularProcess> process; // 실제 event/payload/count/double 기한을 보관한다.
    std::string trace; // 실제 선택한 효과와 전체 인자 순서다.
    // 원본은 할당 필터 없이 현재 물리 슬롯을 읽는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 생성자 경계의 필드 입력만 기존 같은 주소에 설치한다.
    void Seed(std::uint32_t event,std::uint32_t count,float payload,double time) { if (!process) process=std::make_unique<RegularProcess>(event,payload);else { auto* reserved=process.get();std::destroy_at(reserved);std::construct_at(reserved,event,payload); }process->ResetTask(event,time);process->SetInitialCount(count); }
    // 원문의 전체 DWORD 인자를 효과 발생 순서로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) { if (!trace.empty()) trace+=';';trace+=name;
        // 인자 폭을 줄이거나 정렬하지 않는다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 입력 자료만 설치하고 예상 좌표/기한/분기는 계산하지 않는다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);CHECK(row.size()==20);now=std::bit_cast<double>(std::array<std::uint32_t,2>{row[3],row[4]});boss=row[9]!=0;priest=row[16];trace.clear();Seed(row[6],row[7],std::bit_cast<float>(row[8]),-100.5);
        // 부모와 포획 Form의 실제 원문 상태다.
        for (const auto sid:{Sid{50},Sid{51}}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});raw[10]=135;raw[40]=static_cast<std::uint8_t>(row[10]);Put(raw,14,row[14]);Put(raw,18,row[15]); }Put(Raw(Sid{51}),12,row[13],2);
        // 원문 고정 Player 패턴이며 쓰는 owner/위치의 기대 색인을 주입하지 않는다.
        for (std::size_t i=0;i<players.size();++i) players[i]=static_cast<std::uint8_t>(i*17+29);
    }
    // 기존 객체 검색/할당/두 생성자/진단을 각각 독립 효과로 제공한다.
    CarrierTaskHooks TaskHooks() {
        CarrierTaskHooks hooks;
        hooks.find=[this](Sid sid) { Event("Q",{sid.value,0xfff0,0x5100});if (row[18]==5) now=42.5;return row[5] ? process.get() : nullptr; };
        hooks.allocate=[this] { Event("A",{40});return row[19]!=0; };
        hooks.construct=[this](Sid sid,std::uint32_t event,bool shared) { Event("C",{shared ? 61U : 46U,event,sid.value,std::bit_cast<std::uint32_t>(5.0F)});Seed(event,0,5.0F,0);if (row[18]==6) { Seed(0x515a,99,2.5F,0);now=123.25; }return process.get(); };
        hooks.assertion=[this] { Event("!",{0x407});if (row[18]==7) { const auto time=process->Time();Seed(0x6161,77,.5F,time);now=64.25; } };
        return hooks;
    }
    // 시작의 큰 효과만 경계로 두고 마지막 예약은 실제 task 모듈을 사용한다.
    CarrierFallStartHooks StartHooks(const RawCarrierTask& task) {
        CarrierFallStartHooks hooks;
        hooks.hasRegular=[this](Sid sid,std::uint32_t event) { Event("H",{sid.value,event});return row[11]!=0; };
        hooks.movementType=[this](std::uint32_t type) { Event("M",{type,0,1});return row[17]; };
        hooks.removeMovement=[this](Sid sid,std::uint32_t type) { Event("D",{sid.value,type,0,1});if (row[18]==1) { boss=false;Raw(Sid{50})[40]=4;priest=159;Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(42.125F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(43.25F)); } };
        hooks.clearSelection=[this] { Event("S",{0,0,1});Raw(Sid{50})[40]&=static_cast<std::uint8_t>(~4U);if (row[18]==2) { boss=true;priest=159;Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(43.5F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(44.5F));Put(Raw(Sid{51}),12,0x2342,2); } };
        hooks.findKind=[this](Sid sid,std::uint32_t kind) { Event("K",{sid.value,kind,0,0});if (row[18]==3) { boss=true;priest=160;Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(.00001F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(254.99999F));Put(Raw(Sid{51}),12,0x3443,2); }return Sid{static_cast<std::uint16_t>(row[12])}; };
        hooks.releasePriest=[this](Sid sid) { Event("R",{sid.value});if (row[18]==4) { now=88.875;Put(Raw(Sid{51}),12,0,2);Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(55.0F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(66.0F)); } };
        hooks.task=[&task](Sid sid,std::uint32_t event,float delay) { task.Set(sid,event,delay); };return hooks;
    }
    // 원문의 반환값과 현재 필드를 정해진 순서로 기록한다.
    std::string State(std::uint32_t result) const {
        const auto time=std::bit_cast<std::array<std::uint32_t,2>>(process->Time()),clock=std::bit_cast<std::array<std::uint32_t,2>>(now);std::string text;
        // 원문과 같은 DWORD 순서로 비교한다.
        for (const auto value:{result,process->Event(),process->Count(),std::bit_cast<std::uint32_t>(process->Payload()),time[0],time[1],clock[0],clock[1],std::uint32_t(boss),priest,Adler(players)}) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text;
    }
};
}
// 기대값은 독립 원문에서만 읽으며 NULL 실패도 정상 성공과 별도로 재생한다.
TEST_CASE(CarrierFallStart_NativeTaskAndBegin) {
    CarrierStartScene scene;std::size_t count{},nulls{},diagnostics{}; // 원문/실제 할당 실패와 제한된 진단 개수다.
    // 기본 CRT의53비트 원문 전체를 실행하고64비트 관측도 원문에 보존한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_FALL_START_FIXTURE).rows) {
        CHECK(fields.size()==9);if (fields.size()!=9) continue;scene.Prepare(StartNumbers(fields[0]));RawCarrierTask task(scene.pool,scene.now,scene.TaskHooks());RawCarrierFallStart begin(scene.pool,scene.players,scene.boss,scene.priest,scene.StartHooks(task));std::uint32_t result{};std::string status="ok";
        try { if (scene.row[0]==0) task.Set(Sid{50},scene.row[1],std::bit_cast<float>(scene.row[2]));else result=begin.Begin(Sid{50}); }catch (const std::bad_alloc&) { status="null";++nulls; }
        const auto state=scene.State(result),raws=Hex(scene.Raw(Sid{50}))+","+Hex(scene.Raw(Sid{51}));const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if ((status!=fields[1] || trace!=fields[2] || state!=fields[3] || raws!=fields[4]) && diagnostics++<6) std::cout<<"캐리어 시작 입력 "<<fields[0]<<"\n실제 "<<status<<' '<<trace<<' '<<state<<' '<<raws<<"\n원본 "<<fields[1]<<' '<<fields[2]<<' '<<fields[3]<<' '<<fields[4]<<'\n';
        CHECK(status==fields[1]);CHECK(trace==fields[2]);CHECK(state==fields[3]);CHECK(raws==fields[4]);++count;
    }
    CHECK(count==2850 && nulls==480);
}
// 이미 낙하 중인 객체는 다른 경계 없이 반환하며 물리 Player 범위는 쓰기 전에 검증한다.
TEST_CASE(CarrierFallStart_LazyBoundaries) {
    CarrierStartScene scene;scene.Prepare({1,0x5105,0,0,0,1,0x5101,7,0,1,0,1,51,0x1281,0,0,158,35,0,1});CarrierFallStartHooks hooks;hooks.hasRegular=[](Sid,std::uint32_t) { return true; };RawCarrierFallStart begin(scene.pool,scene.players,scene.boss,scene.priest,std::move(hooks));CHECK(begin.Begin(Sid{50}));CHECK(&begin.Pool()==&scene.pool);
    RawCarrierTask missing(scene.pool,scene.now,{});bool rejected{};try { missing.Set(Sid{50},0x5105,0); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    scene.row[11]=0;scene.boss=false;Put(scene.Raw(Sid{51}),12,0x7f,2);const auto before=Adler(scene.players);RawCarrierTask task(scene.pool,scene.now,scene.TaskHooks());RawCarrierFallStart limited(scene.pool,std::span(scene.players).first(9*0xb4),scene.boss,scene.priest,scene.StartHooks(task));
    rejected=false;try { static_cast<void>(limited.Begin(Sid{50})); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);CHECK(Adler(scene.players)==before);
}
// 웜40 생성자 전체100080바이트와 공유 본문/다른 부착·부모 Halt를 원문으로 검증한다.
TEST_CASE(CarrierFallStart_WormNativeConstructorAndVtables) {
    std::size_t rows{},created{}; // 세 heap 패턴과 각각의 실제 할당 결과다.
    // 성공은 제품 생성자 전체를 비교하고 실패는 원문이 heap에 쓰지 않은 것을 확인한다.
    for (const auto& fields:LoadFixture(NETSTORM_WORM_PATH_FIXTURE).rows) {
        CHECK(fields.size()==6);if (fields.size()!=6) continue;const auto fill=static_cast<std::uint8_t>(Number(fields[0]));const bool allocate=Number(fields[1])!=0;
        if (allocate) { RawPathRuntime runtime(OriginalEdition::Patch1078,false,fill,false,true);CHECK(Number(fields[2])==1 && Get(runtime.Bytes(),0)==0x50e4c4U);CHECK(Adler(runtime.Bytes())==Number(fields[3]));++created; }
        else { const std::vector<std::uint8_t> heap(0x186f0,fill);CHECK(Number(fields[2])==0 && Adler(heap)==Number(fields[3])); }++rows;
    }
    CHECK(rows==6 && created==3);std::map<std::uint32_t,std::vector<std::uint32_t>> tables; // 실제 두 프로세스/두 부모 가상 표 원문이다.
    // native 주소를 호스트 함수로 실행하지 않고 선택 근거로만 읽는다.
    for (const auto& fields:LoadFixture(NETSTORM_WORM_PATH_VTABLES).rows) { std::vector<std::uint32_t> row;
        // 원문 슬롯의 전체 DWORD를 보존한다.
        for (const auto& value:fields) row.push_back(Number(value));if (!row.empty()) tables.emplace(row[0],std::move(row));
    }
    CHECK(tables.size()==4);const auto& worm=tables.at(40);const auto& priest=tables.at(54);CHECK(worm.size()==15 && priest.size()==15);
    // 실제13슬롯 중 부착2c만 다르므로 frame/Init/도착/삭제/Prepare는 같은 본문이다.
    for (std::size_t index=2;index<15;++index) if (index!=13) CHECK(worm[index]==priest[index]);
    CHECK(worm[1]==0x50e4c4U && worm[13]==0x48bde0U && priest[13]==0x48bd30U);CHECK(worm[8]==0x48bb40U && worm[10]==0x48bb10U && worm[11]==0x48bd90U);
    const auto& sun=tables.at(133);const auto& empty=tables.at(187);CHECK(sun.size()==7 && empty.size()==7);CHECK(sun[1]==0x50e568U && sun[2]==0x42a5f0U);CHECK(empty[1]==0x503ff0U && empty[2]==0x42ac60U);
    // 두 부모의 도착/이동 통지/낙하는 공유하지만 empty의 주기적 Halt는 별도다.
    for (std::size_t index=3;index<7;++index) CHECK(sun[index]==empty[index]);
}
