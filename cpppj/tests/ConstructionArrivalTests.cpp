// teleportEffect 도착의 두 실제 PE fixture와 현재 후보/서버 경계/다중 건설 시작을 대조한다.
#include "TestSupport.h"
#include "o/RawConstructionArrival.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 도착 운반 객체의 fixture SID다.
constexpr Sid kBuilder{50};
// 마지막 빈 항목도 보존하는 fixture 구분자 판독이다.
std::vector<std::string> Split(std::string_view value,char separator) {
    std::vector<std::string> result;std::size_t start=0;
    // 같은 저장 순서의 모든 항목을 읽는다.
    for (;;) {
        const auto end=value.find(separator,start);result.emplace_back(value.substr(start,end==std::string_view::npos ? end : end-start));
        if (end==std::string_view::npos) return result;
        start=end+1;
    }
}
// DWORD의 부호 없는 십진수 표현을 좁힌다.
std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoull(value)); }
// 정렬되지 않은 little endian raw 필드를 지정 폭으로 쓴다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 지정 필드 이외의 바이트는 바꾸지 않는다.
    for (std::size_t byte=0;byte<width;++byte) raw[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 실패를 호스트 예외로 보고하는 계약을 검사한다.
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
// 같은 풀의 client/server 슬롯을 미리 확보하고 현재 raw/해시를 합성 입력으로 만든다.
struct ArrivalScene {
    SidPool pool;SquidHash hash;std::vector<RiftTypeRecord> types;ConstructionArrivalState state;
    std::map<std::uint16_t,std::span<std::uint8_t>> slots;std::string events;std::uint32_t mutation{};bool mutated{};
    // 실제 SID 서버 경계 앞/뒤 입력을 지원한다. captured span은 free 상태 입력도 쓰게 한다.
    explicit ArrivalScene(OriginalEdition edition):pool(edition,32768,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {
        const auto first=pool.Layout().serverFirst;
        // client 영역의 예약 꼬리 앞까지 실제 할당하고 테스트가 사용하는 슬롯만 보관한다.
        for (std::uint32_t id=5;id<first-1;++id) {
            const auto sid=pool.Allocate(2);CHECK(sid.value==id);
            if (id==kBuilder.value || id==60) slots.emplace(sid.value,pool.AllocatedBytes(sid));
        }
        // 예약 꼬리의 합성 raw 입력은 일반 할당의 결과가 아니다. 실제 비const 풀 저장소에 입력만 공급한다.
        const auto tail=pool.Slot(Sid{static_cast<std::uint16_t>(first-1)});
        slots.emplace(static_cast<std::uint16_t>(first-1),std::span<std::uint8_t>(const_cast<std::uint8_t*>(tail.data()),tail.size()));
        // 서버 영역의 첫 세 번호를 별도로 실제 할당한다.
        for (std::uint32_t id=first;id<first+3;++id) {
            const auto sid=pool.Allocate();CHECK(sid.value==id);slots.emplace(sid.value,pool.AllocatedBytes(sid));
        }
    }
    // 판본별 abstract bit의 raw 위치다.
    std::size_t Extra() const { return pool.Edition()==OriginalEdition::Patch1078 ? 40 : 35; }
    // 효과를 한 줄에 순서대로 연결한다.
    void Event(std::string value) { if (!events.empty()) events+=';';events+=value; }
    // fixture에 저장된 현재 전역/모든 후보를 원본 입력 순서로 등록한다.
    void Setup(const std::string& input,const std::string& description) {
        const auto parameters=Split(input,',');CHECK(parameters.size()==6);events.clear();mutated=false;hash.Reset();
        state.checking=Number(parameters[2])!=0;state.streakProcessType=Number(parameters[3]);state.duration=std::bit_cast<float>(Number(parameters[4]));mutation=Number(parameters[5]);
        std::fill(types.begin(),types.end(),RiftTypeRecord{});
        // 남은 옛 체인/raw 필드가 다음 합성 장면에 영향을 주지 않는다.
        for (auto& [sid,raw]:slots) { (void)sid;std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        auto builder=slots.at(kBuilder.value);Write(builder,14,Number(parameters[0]));Write(builder,18,Number(parameters[1]));
        if (description=="-") return;
        // 입력 순서대로 동일한 네 단계 hash 머리에 연결한다.
        for (const auto& text:Split(description,';')) {
            const auto fields=Split(text,',');CHECK(fields.size()==10);std::array<std::uint32_t,10> values{};
            // 모든 좌표/상태는 native fixture의 DWORD/BYTE 비트값이다.
            for (std::size_t i=0;i<values.size();++i) values[i]=Number(fields[i]);
            auto raw=slots.at(static_cast<std::uint16_t>(values[0]));raw[10]=static_cast<std::uint8_t>(values[1]);raw[11]=static_cast<std::uint8_t>(values[2]);
            raw[Extra()]=static_cast<std::uint8_t>(values[3]);Write(raw,14,values[4]);Write(raw,18,values[5]);
            auto& type=types.at(values[1]);type.footX=static_cast<int>(values[6]);type.footY=static_cast<int>(values[7]);type.flags1=values[8];
            auto& head=hash.Bucket(static_cast<int>(values[9]),std::bit_cast<float>(values[4]),std::bit_cast<float>(values[5]));
            Write(raw,4,head,2);head=static_cast<std::uint16_t>(values[0]);
        }
    }
    // 실제 PE의 세 하위 효과만 동일하게 기록하고 첫 start 뒤의 현재 자료 변형을 적용한다.
    ConstructionArrivalHooks Hooks() {
        ConstructionArrivalHooks hooks;
        hooks.start=[this](Sid sid,float duration) {
            Event("S:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(duration)));
            if (mutated) return;mutated=true;const auto first=pool.Layout().serverFirst;
            const auto target=static_cast<std::uint16_t>(sid.value==first+1 ? first+2 : first+1);auto raw=slots.at(target);
            if (mutation==1) raw[Extra()]=0;
            else if (mutation==2) types.at(target==first+1 ? 100 : 101).flags1=0;
            else if (mutation==3) Write(slots.at(sid.value),4,0,2);
            else if (mutation==4) Write(raw,14,std::bit_cast<std::uint32_t>(200.0F));
            else if (mutation==5) state.duration=0.9F;
        };
        hooks.removeContained=[this](Sid sid,std::uint32_t type) { Event("R:"+std::to_string(sid.value)+':'+std::to_string(type)+":0:1"); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event("K:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        return hooks;
    }
};
// 한 판본의 고유 관찰을 모두 재생한다. 원본 PE/디컴파일/Python은 필요 없다.
void Replay(std::string_view edition) {
    std::ifstream input(NETSTORM_CONSTRUCTIONARRIVAL_FIXTURE);if (!input) throw std::runtime_error("건설 도착 fixture 없음");
    ArrivalScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::string line;std::size_t count=0;
    RawConstructionArrival arrival(scene.pool,scene.hash,scene.types,scene.state,scene.Hooks());
    // 해당 판본의 정상/예측/무대상/변형 관찰만 대조한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');CHECK(row.size()==6);if (row[0]!=edition) continue;
        scene.Setup(row[1],row[2]);CHECK(arrival.Arrive(kBuilder)==Number(row[3]));CHECK((scene.events.empty() ? "-" : scene.events)==row[5]);++count;
    }
    CHECK(count==153);
}
}
// 10.78 도착의 실제 반환/하위 효과·current 후보/서버 경계를 대조한다.
TEST_CASE(ConstructionArrival_ReplaysPatch) { Replay("originals"); }
// CD 대응 도착의 실제 반환/하위 효과·current 후보/서버 경계를 대조한다.
TEST_CASE(ConstructionArrival_ReplaysCd) { Replay("originalCD"); }
// 누락된 연결과 타입 표가 달라진 월드를 첫 효과 전에 거부한다.
TEST_CASE(ConstructionArrival_RequiresCurrentWorldAndEffects) {
    ArrivalScene scene(OriginalEdition::Patch1078);auto hooks=scene.Hooks();hooks.destroy={};
    CHECK(Throws([&] { RawConstructionArrival bad(scene.pool,scene.hash,scene.types,scene.state,hooks); }));
    auto shortTypes=std::span<const RiftTypeRecord>(scene.types).first(171);
    CHECK(Throws([&] { RawConstructionArrival bad(scene.pool,scene.hash,shortTypes,scene.state,scene.Hooks()); }));
}
