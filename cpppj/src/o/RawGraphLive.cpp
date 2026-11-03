// Graph의 실제 부모는 현재 raw 풀/기록/스택을 직접 소비한다. 순수 Calculate는 사전 검사에만 남긴다.
#include "o/RawGraph.h"
#include "o/RawSquidNeighbors.h"
#include "o/RawSquidFinder.h"
#include "o/RawPathSearch.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::o { // 판본별 원문 Graph 실행과 같은 장기 물리를 연결한다.
namespace { // 포인터/정렬과 무관한 원문 정수 폭과 float 위치 판독이다.
// 실제 TYPE 로딩 순서의 두 비표면 graph BYTE 예외다. 두 판본에서 같은 번호다.
constexpr std::uint8_t kFenceMark=90,kWindArcher=140;
// 원문의 WORD 저장 지점마다 low16비트와 signed 해석을 유지한다.
std::int16_t GraphWord(std::int32_t value) { return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value)); }
// 정렬되지 않은 슬롯 좌표 DWORD를 float32로 읽는다.
float GraphCoordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 원문 little endian 순서로 네 바이트를 조합한다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return std::bit_cast<float>(value);
}
}
// 선택된 원문 진단만 장치를 요구한다. CD의 로그/무조건 assertion 파일 이름은 소문자다.
void RawGraph::Diagnose(std::string_view expression,std::uint32_t line,std::string_view file) const {
    if (!diagnostics_.assertion) throw std::logic_error("Graph 실제 진단 장치 미연결");
    if (file.empty()) file=pool_.Edition()==OriginalEdition::Patch1078 ? "\\Ns\\O\\Graph.cpp" : "\\ns\\o\\graph.cpp";
    diagnostics_.assertion(expression,file,line);
}
// PATCH의 raw getter는 TYPE 검사를 무조건 수행하고 CD는 graph BYTE만 읽는다.
std::uint8_t RawGraph::ReadNumber(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto type=raw[10];
    if (pool_.Edition()==OriginalEdition::Patch1078 && !(types_.at(type).flags1&TypeFlag1::kSurface) && type!=kFenceMark && type!=kWindArcher)
        Diagnose("getTypeFlags() & TYPE_FLAG_IS_SURFACE || getType() == fencemarkType || getType() == windArcherType",0x739,"\\Ns\\O\\Squid.cpp");
    return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 30 : 28];
}
// PATCH는 타입/번호를 진단한 뒤 현재 graph BYTE를 쓴다. 진단으로 free가 바뀌어도 추가 수명 관문을 만들지 않는다.
void RawGraph::WriteNumber(Sid sid,std::uint8_t graph) const {
    const auto type=pool_.Slot(sid)[10];
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        if (!(types_.at(type).flags1&TypeFlag1::kSurface) && type!=kFenceMark && type!=kWindArcher)
            Diagnose("getTypeFlags() & TYPE_FLAG_IS_SURFACE || type == fencemarkType || type == windArcherType",0x74b,"\\Ns\\O\\Squid.cpp");
        if (graph>=Graph::kCount && graph!=Graph::kInvalid && type!=kFenceMark && type!=kWindArcher)
            Diagnose("( n >=0 && n < MaxGraphs) || ( n==invalidGraphNum) || type == fencemarkType || type == windArcherType",0x750,"\\Ns\\O\\Squid.cpp");
    }
    pool_.bytes_[pool_.Offset(sid)+(pool_.Edition()==OriginalEdition::Patch1078 ? 30 : 28)]=graph;
}
// 로그 장치가 없으면 독립 입력의 기존 무출력 계약을 유지한다. 실제 Client는 자체 필터/FILE writer를 연결한다.
void RawGraph::TraceInUse() const {
    if (!trace_) return;
    const auto used=std::count_if(records_.begin(),records_.begin()+Graph::kCount,[](const auto& record) { return record.inUse!=0; });
    trace_(0x20,std::to_string(used)+" graphs in use\n");
}
// 같은 Client 수명의 현재 로그 장치를 보관하며 그래프 물리에는 쓰지 않는다.
void RawGraph::BindTrace(std::function<void(std::int32_t,std::string_view)> trace) { trace_=std::move(trace); }
// 실제 World의 활성/분할 전역을 같은 현재 주소에서 읽는다. 임시값은 RawCheckingFlag가 소유한다.
void RawGraph::BindDetachState(RawCheckingFlag enabled,RawCheckingFlag rebuild) { detachEnabled_=enabled;detachRebuild_=rebuild; }
// 삭제 부모가 호출한 원문 두 시각 읽기를 실제 Client 시계와 연결한다.
void RawGraph::BindClock(std::function<double()> clock) { clock_=std::move(clock); }
// 현재 낮은 빈 행을 선택하며 소진한 경우에만 전체 재구성을 수행한다.
std::uint8_t RawGraph::ExecuteAllocate(bool recover) {
    auto found=std::find_if(records_.begin(),records_.begin()+Graph::kCount,[](const auto& record) { return !record.inUse; });
    if (found==records_.begin()+Graph::kCount) {
        if (!recover) throw std::logic_error("그래프가 소진되어 전체 풀 재구성이 필요합니다");
        ExecuteRebuild(true);
        found=std::find_if(records_.begin(),records_.begin()+Graph::kCount,[](const auto& record) { return !record.inUse; });
        if (found==records_.begin()+Graph::kCount) {
            Diagnose("false",0x25d);throw std::logic_error("원문 Graph 할당의 NULL 반환은 호스트 레코드로 해석할 수 없습니다");
        }
    }
    found->surfaces=0;found->inUse=1;return static_cast<std::uint8_t>(found-records_.begin());
}
// 현재 표/슬롯/stack에서 실행하며 진단과 로그가 고친 값을 사본 Commit으로 덮지 않는다.
std::uint32_t RawGraph::ExecuteFlood(Sid sid,std::uint8_t graph,bool inspect) {
    CheckGraphSentinel(records_[Graph::kInvalid],pool_.Edition(),diagnostics_);ValidateFree(graph);
    if (inspect) static_cast<void>(Calculate(sid,Operation::Inspect,0,nullptr)); // 외부 입력의 기존 물리 보호만 유지한다. 내부 재구성/병합은 원문 현재 상태를 직접 소비한다.
    std::vector<RiftTypeFrames> codes;codes.reserve(frames_.size());
    // 원문 프레임 코드를 기존 raw 이웃 탐색기에 연결한다. 번호/상태/지도는 복사하지 않는다.
    for (const auto& frames:frames_) codes.emplace_back(frames);
    RawSquidNeighbors neighbors(pool_,hash_,spots_,types_,codes);
    stack_[0]=sid.value;std::size_t depth=1;std::uint32_t changed=0;
    // 별도 방문 집합 없이 현재 목적 번호를 읽고 원문 LIFO 중복 push를 유지한다.
    while (depth) {
        const auto value=stack_[--depth];
        if (!value || value>std::numeric_limits<std::uint16_t>::max() || value>=pool_.Capacity()) throw std::out_of_range("Graph 현재 flood stack SID 범위 오류");
        const Sid current{static_cast<std::uint16_t>(value)};const auto previous=ReadNumber(current);
        if (previous==graph) continue;
        ValidateFree(previous);++changed;
        if (previous!=Graph::kInvalid) records_[previous].surfaces=GraphWord(records_[previous].surfaces-1);
        if (graph!=Graph::kInvalid) records_[graph].surfaces=GraphWord(records_[graph].surfaces+1);
        WriteNumber(current,graph);
        RawSquidNeighborWalk walk(neighbors,current,NeighborFlag::kSurfaceMap);
        // Next마다 현재 지도/후보를 읽는다. 선택된 진단이 슬롯/stack을 고친 뒤에도 순서를 계속한다.
        for (auto candidate=walk.Current();candidate.value;candidate=walk.Next()) {
            if (ReadNumber(candidate)==graph) continue;
            if (pool_.Edition()==OriginalEdition::Patch1078 && diagnostics_.checking && candidate==current)
                Diagnose("cFinder.s().getSid() != sid",0x176);
            if (depth>=stack_.size()) { Diagnose("d < FLOOD_STACK_SIZE",0x17a);throw std::out_of_range("Graph flood의 원문 물리 stack 범위를 넘습니다"); }
            stack_[depth++]=candidate.value;
            if (depth>=stack_.size()) Diagnose("d < FLOOD_STACK_SIZE",0x17a);
        }
    }
    TraceInUse();CheckGraphSentinel(records_[Graph::kInvalid],pool_.Edition(),diagnostics_);return changed;
}
// 재구성은 초기 sentinel 진단 뒤 필요한 Reset, SID 순서 두 순회, 임시 행 해제와 마지막 sentinel 순서다.
void RawGraph::ExecuteRebuild(bool resetAll) {
    CheckGraphSentinel(records_[Graph::kInvalid],pool_.Edition(),diagnostics_);
    const bool exhausted=std::none_of(records_.begin(),records_.begin()+Graph::kCount,[](const auto& record) { return !record.inUse; });
    static_cast<void>(Calculate(Sid{},!resetAll && exhausted ? Operation::Allocate : Operation::Rebuild,resetAll ? 1 : 0,nullptr));
    if (resetAll) ResetRawGraphTable(storage_);
    const auto temporary=ExecuteAllocate(true);const auto firstLimit=pool_.Capacity();
    // 원문 iterator는 초기1에서 Next 선증가로2부터 시작하고 free만 건너뛴다. surface의 void/dead/contained는 제외하지 않는다.
    for (std::uint32_t id=2;id<firstLimit;++id) {
        const Sid sid{static_cast<std::uint16_t>(id)};const auto raw=pool_.Slot(sid);
        if (raw[11]&1) continue;const auto& type=types_.at(raw[10]);if (!(type.flags1&TypeFlag1::kSurface)) continue;
        if (!resetAll && ReadNumber(sid)!=Graph::kInvalid) continue;
        const auto x=GraphCoordinate(raw,14),y=GraphCoordinate(raw,18);
        if (!std::isfinite(x) || !std::isfinite(y) || x<0 || y<0 || x>=256 || y>=256) throw std::out_of_range("Graph 재구성 spot 좌표 범위 오류");
        if (!(spots_[static_cast<std::size_t>(static_cast<int>(y)*256+static_cast<int>(x))]&8)) {
            WriteNumber(sid,temporary);records_[temporary].surfaces=GraphWord(records_[temporary].surfaces+1);
        }
    }
    const auto secondLimit=pool_.Capacity();
    // 다시 초기1→Next로2부터 읽고 앞 flood가 바꾼 현재 번호를 확인해 다음 성분의 번호를 할당한다.
    for (std::uint32_t id=2;id<secondLimit;++id) {
        const Sid sid{static_cast<std::uint16_t>(id)};const auto raw=pool_.Slot(sid);
        if (!(raw[11]&1) && (types_.at(raw[10]).flags1&TypeFlag1::kSurface) && ReadNumber(sid)==temporary) {
            const auto number=ExecuteAllocate(true);ExecuteFlood(sid,number);
        }
    }
    records_[temporary].surfaces=0;records_[temporary].inUse=0;
    CheckGraphSentinel(records_[Graph::kInvalid],pool_.Edition(),diagnostics_);
}
// 원문 진단 후 현재 이웃 번호를 수집하고 가장 큰 첫 연결에 붙인 뒤 나머지 현재 무리를 병합한다.
void RawGraph::ExecuteAdd(Sid sid) {
    CheckGraphSentinel(records_[Graph::kInvalid],pool_.Edition(),diagnostics_);
    const auto raw=pool_.Slot(sid);const auto x=GraphCoordinate(raw,14),y=GraphCoordinate(raw,18);
    const bool valid=std::isfinite(x) && std::isfinite(y) && x>0 && y>0 && x<256 && y<256;
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch && diagnostics_.checking && !valid) Diagnose("pos.isValid()",0xbd);
    if (!valid || (!patch && (static_cast<int>(x)<=0 || static_cast<int>(y)<=0))) return;
    if (patch && diagnostics_.checking && (pool_.Slot(sid)[11]&7)) Diagnose("!s.isVoid() && !s.isDead() && !s.isFree()",0xc2);
    if (pool_.Slot(sid)[11]&7) return;
    static_cast<void>(Calculate(sid,Operation::Inspect,0,nullptr));
    std::vector<RiftTypeFrames> codes;codes.reserve(frames_.size());
    // 원천의 캐시와 각 후보의 현재 frame을 같은 코드 표에 연결한다.
    for (const auto& frames:frames_) codes.emplace_back(frames);
    RawSquidNeighbors neighbors(pool_,hash_,spots_,types_,codes);RawSquidNeighborWalk walk(neighbors,sid,NeighborFlag::kSurfaceMap);
    std::array<Sid,64> connections{};std::size_t count=0;
    std::int32_t biggestSize=std::numeric_limits<std::int32_t>::min()+1;std::uint8_t biggest=0; // 원문 0x80000001의 크기 초기값이며 동률에서는 첫 연결을 유지한다.
    // 원문은 번호를 snapshot하지 않고 이미 넣은 SID의 현재 graph BYTE와 중복을 비교한다.
    for (auto candidate=walk.Current();candidate.value;candidate=walk.Next()) {
        const auto number=ReadNumber(candidate);if (number==Graph::kInvalid) continue;ValidateFree(number);
        if (std::any_of(connections.begin(),connections.begin()+count,[&](auto prior) { return ReadNumber(prior)==ReadNumber(candidate); })) continue;
        if (count==connections.size()) throw std::out_of_range("Graph 연결 목록의 원문 물리 범위를 넘습니다");
        connections[count++]=candidate;
        const auto current=ReadNumber(candidate);ValidateFree(current);
        if (records_[current].surfaces>biggestSize) { biggest=current;biggestSize=records_[current].surfaces; }
    }
    if (!count) {
        const auto number=ExecuteAllocate(recovery_==GraphRecovery::FullPool);
        if (records_[number].inUse!=1) Diagnose("g->inUse == true",0xf3);
        ExecuteFlood(sid,number);
    } else {
        auto& winner=records_[biggest];
        if (patch && diagnostics_.checking && winner.inUse!=1) Diagnose("g->inUse == true",0x100);
        if (!winner.inUse) winner.inUse=1;
        WriteNumber(sid,biggest);winner.surfaces=GraphWord(winner.surfaces+1);
        if (count>1) {
            std::size_t flooded=0;
            // 같은 지역 배열64칸을 순회하며 앞 flood/진단/로그가 바꾼 뒤 후보 번호를 다시 읽는다.
            for (auto candidate:connections) {
                if (!candidate.value || ReadNumber(candidate)==biggest) continue;
                const auto loser=ReadNumber(candidate);
                if (loser==Graph::kInvalid) throw std::out_of_range("Graph 병합의 원문 loser NULL 행입니다");ValidateFree(loser);
                auto& record=records_[loser];
                if (patch && diagnostics_.checking && record.surfaces>biggestSize) Diagnose("loser->numSurfaces <= biggestSize",0x113);
                ExecuteFlood(candidate,biggest);++flooded;record.surfaces=0;record.inUse=0;
            }
            if (patch && diagnostics_.checking && flooded!=count-1) Diagnose("numFlooded == numConnections - 1",0x11f);
        }
    }
    TraceInUse();
}
// 일반 finder는 다음 raw 체인을 먼저 저장한다. Add의 진단/로그 뒤 다음 후보의 현재 필드를 다시 읽는다.
void RawGraph::ExecutePostDestroy(Sid sid) {
    ValidatePostDestroy(sid);
    const auto raw=pool_.Slot(sid);const auto& type=types_.at(raw[10]);
    const auto x=GraphCoordinate(raw,14),y=GraphCoordinate(raw,18);
    if (!std::isfinite(x) || !std::isfinite(y) || x<0 || y<0 || x>=256 || y>=256) throw std::out_of_range("Graph postDestroy 좌표 범위 오류");
    const auto left=std::clamp(static_cast<int>(x)-type.footX+1,1,255),top=std::clamp(static_cast<int>(y)-type.footY+1,1,255);
    const SquidSearchArea area{left,top,x>0 && y>0 ? static_cast<int>(x) : left,x>0 && y>0 ? static_cast<int>(y) : top};
    RawSquidFinder finder(pool_,hash_,types_);
    // 이전 Add가 고친 후보 타입/상태와 현재 hash/next를 읽으며 표면만 Add한다.
    for (auto candidate=finder.Begin(area);candidate.value;candidate=finder.Next()) {
        if (types_.at(pool_.Slot(candidate)[10]).flags1&TypeFlag1::kSurface) Add(candidate);
    }
}
// 원문 삭제 정보는 sentinel 전 인자 값이다. 사전 검사는 기존 안전 계약만 유지하고 효과를 commit하지 않는다.
void RawGraph::ExecuteDetach(Sid sid,bool rebuild,std::uint8_t removedSurfaces,std::optional<std::uint8_t> sourceType) {
    if (!detachEnabled_) return;
    ValidateDetach(sid,detachRebuild_ ? static_cast<bool>(*detachRebuild_) : rebuild,removedSurfaces,sourceType);
    std::vector<RiftTypeFrames> codes;codes.reserve(frames_.size());
    // 후보 타입/프레임은 바꾸지 않고 값 원천만 삭제 정보의 타입으로 치환한다.
    for (const auto& frames:frames_) codes.emplace_back(frames);
    RawSquidNeighbors neighbors(pool_,hash_,spots_,types_,codes);auto source=neighbors.Source(sid);
    if (sourceType) source.type=*sourceType;
    CheckGraphSentinel(records_[Graph::kInvalid],pool_.Edition(),diagnostics_);
    if (!detachEnabled_) return; // GetGraph(position)도 현재 활성 전역을 다시 읽는다.
    const auto head=RawPathSurfaceAt(hash_,{source.x,source.y});if (!head.value) return;
    const auto number=ReadNumber(head);if (number==Graph::kInvalid) return;ValidateFree(number);
    auto& original=records_[number];if (!original.inUse) return;
    RawSquidNeighborWalk walk(neighbors,source,0,true);std::array<Sid,64> connections{};std::size_t count=0;
    // 원문은 그래프별 중복 제거 없이 일반 finder의 현재 SID를 모두 저장한다.
    for (auto candidate=walk.Current();candidate.value;candidate=walk.Next()) {
        if (count==connections.size()) throw std::out_of_range("Graph 삭제 연결의 원문 물리 범위를 넘습니다");
        connections[count++]=candidate;
        if (count>=connections.size()) Diagnose("numConnections < MAX_CONNECTIONS",0x1ef);
    }
    if (clock_) static_cast<void>(clock_());
    // keep의 초기값은 현재 전역을 한 번 읽지만 각 연결의 분할 조건은 Flood 뒤 다시 읽는다.
    const auto rebuilding=[&] { return detachRebuild_ ? static_cast<bool>(*detachRebuild_) : rebuild; };
    bool keep=!rebuilding();std::size_t remaining=count;
    // 후보 graph는 앞 flood/로그/진단이 고친 현재 슬롯에서 읽으며 초기 목록의 순서는 보존한다.
    for (std::size_t index=0;index<count;++index,--remaining) {
        const auto candidate=connections[index];if (!candidate.value) Diagnose("floodSid != 0",0x20b);
        std::uint8_t current{};
        if (!candidate.value) {
            if (pool_.Edition()==OriginalEdition::Patch1078 && diagnostics_.checking) Diagnose("mySid",0x93);
        } else if (types_.at(pool_.Slot(candidate)[10]).flags1&TypeFlag1::kSurface) current=ReadNumber(candidate);
        else {
            const auto raw=pool_.Slot(candidate);const auto surface=RawPathSurfaceAt(hash_,{GraphCoordinate(raw,14),GraphCoordinate(raw,18)});
            current=surface.value ? ReadNumber(surface) : Graph::kInvalid;
        }
        ValidateFree(current);if (current!=number) continue;
        if (!rebuilding() && remaining<2) { if (!(types_.at(source.type).flags2&8)) keep=true; }
        else { const auto allocated=ExecuteAllocate(recovery_==GraphRecovery::FullPool);ExecuteFlood(candidate,allocated); }
    }
    if (count) original.surfaces=GraphWord(original.surfaces-removedSurfaces);
    if (!keep) { original.surfaces=0;original.inUse=0; }
    if (clock_) static_cast<void>(clock_());
    CheckGraphSentinel(records_[Graph::kInvalid],pool_.Edition(),diagnostics_);
}

}
