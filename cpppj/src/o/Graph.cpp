// 원본 Graph.cpp의 기존 표/스택·소진 전 경로. 전역 소진 복구와 실제 월드 수명은 후속이다.
#include "o/Graph.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 WORD 증가/감소의 low 16비트와 signed 해석을 보존한다.
std::int16_t Word(std::int32_t value) { return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value)); }
}
// 모든 membership의 표면/상태/번호를 확인하며 판독한 스냅샷을 변경하지 않는다.
Graph::Graph(const SurfaceFinder& surfaces,std::span<const GraphMembership> members,std::span<const GraphRecord> records,
    std::span<const std::uint32_t> stack)
    :surfaces_(surfaces) {
    if (!records.empty() && records.size()!=kTableSize) throw std::invalid_argument("그래프 표 크기 오류");
    if (!stack.empty() && stack.size()!=kFloodSize) throw std::invalid_argument("그래프 스택 크기 오류");
    if (!stack.empty()) std::copy(stack.begin(),stack.end(),stack_.begin());
    if (records.empty()) records_[kInvalid]={0x7dfd,0x7dfd,0};
    else std::copy(records.begin(),records.end(),records_.begin());
    // 입력 객체는 이미 확보한 실제 표면으로 한정한다. 비표면의 위치 조회 경로는 별도다.
    for (auto member:members) {
        CheckNumber(member.graph,true); const auto& object=surfaces_.Object(member.id);
        if (!(object.flags1&TypeFlag1::kSurface) || object.dead!=((member.state&2)!=0) ||
            !members_.emplace(member.id,member).second) throw std::invalid_argument("그래프 표면/상태/중복 번호 오류");
    }
}
// 251..253은 정상 그래프도 sentinel도 아니다.
void Graph::CheckNumber(std::uint8_t graph,bool allowInvalid) {
    if (graph>=kCount && !(allowInvalid && graph==kInvalid)) throw std::out_of_range("그래프 번호 오류");
}
// 원본은 낮은 번호부터 찾으며 사용 중/미사용 기록의 reserved WORD를 건드리지 않는다.
std::uint8_t Graph::Allocate() {
    // 소진은 원본의 전체 재구성/삭제 단계가 필요하므로 기존 표를 변경하지 않는다.
    for (std::size_t i=0;i<kCount;++i) if (!records_[i].inUse) {
        records_[i].surfaces=0; records_[i].inUse=1; return static_cast<std::uint8_t>(i);
    }
    throw std::logic_error("그래프 소진 복구 미복원");
}
// 무효 번호의 sentinel과 reserved WORD를 보존한다.
void Graph::Free(std::uint8_t graph) {
    CheckNumber(graph,true); if (graph==kInvalid) return;
    records_[graph].surfaces=records_[graph].inUse=0;
}
// debug assert UI를 실행하지 않는 원본 일반 경로다.
void Graph::Remove(std::uint8_t graph) {
    CheckNumber(graph,false); auto& r=records_[graph];
    if (r.inUse && r.surfaces>0) { r.surfaces=Word(r.surfaces-1); if (!r.surfaces) r.inUse=0; }
}
// membership 밖 지도/스택 과다 입력은 원본의 부분 변경을 게임 월드에 전파하지 않는다.
std::uint32_t Graph::Flood(std::uint16_t id,std::uint8_t graph) {
    CheckNumber(graph,true); auto next=*this; const auto result=next.FloodImpl(id,graph);
    records_=next.records_; stack_=next.stack_; members_=std::move(next.members_); return result;
}
// 원본처럼 먼저 현재 객체의 번호를 바꾼 뒤 y/x 순서 이웃을 push하고 마지막 이웃부터 방문한다.
std::uint32_t Graph::FloodImpl(std::uint16_t id,std::uint8_t graph) {
    static_cast<void>(members_.at(id)); stack_[0]=id; std::size_t depth=1; std::uint32_t changed=0;
    // 방문 번호가 이미 목적 그래프라면 이웃을 확장하지 않는다. 별도 visited 표는 사용하지 않는다.
    while (depth) {
        const auto current=static_cast<std::uint16_t>(stack_[--depth]); auto& member=members_.at(current);
        if (member.graph==graph) continue;
        if (member.graph!=kInvalid) records_[member.graph].surfaces=Word(records_[member.graph].surfaces-1);
        if (graph!=kInvalid) records_[graph].surfaces=Word(records_[graph].surfaces+1);
        member.graph=graph; ++changed;
        // 동일한 목적 번호가 아닌 이웃은 중복 push도 허용한다.
        for (auto neighbor:surfaces_.Neighbors(current)) if (members_.at(neighbor).graph!=graph) {
            if (depth+1>=kFloodSize) throw std::out_of_range("그래프 flood 스택 범위 오류");
            stack_[depth++]=neighbor;
        }
    }
    return changed;
}
// 입력 오류/미복원 소진은 원본 위치/그래프를 쓰기 전에 처리한다.
void Graph::Add(std::uint16_t id) {
    auto next=*this; next.AddImpl(id); records_=next.records_; stack_=next.stack_; members_=std::move(next.members_);
}
// 유효/활성 표면만 처리하고 가장 큰 연결 그래프의 동률에서는 첫 탐색 결과를 유지한다.
void Graph::AddImpl(std::uint16_t id) {
    auto& source=members_.at(id); const auto& object=surfaces_.Object(id);
    if (object.x<=0 || object.y<=0 || object.x>=kWorldCells || object.y>=kWorldCells || (source.state&7)) return;
    std::vector<std::uint16_t> connections; int biggestSize=std::numeric_limits<int>::min(); std::uint8_t biggest=0;
    // 탐색 순서대로 그래프 번호 중복을 제거한다. 이웃 객체 번호가 다른 경우도 한 연결이다.
    for (auto neighbor:surfaces_.Neighbors(id)) {
        const auto graph=members_.at(neighbor).graph;
        if (graph==kInvalid || std::any_of(connections.begin(),connections.end(),[&](auto n){return members_.at(n).graph==graph;})) continue;
        if (connections.size()==64) throw std::out_of_range("그래프 연결 목록 범위 오류");
        connections.push_back(neighbor);
        if (records_[graph].surfaces>biggestSize) { biggest=graph; biggestSize=records_[graph].surfaces; }
    }
    if (connections.empty()) { FloodImpl(id,Allocate()); return; }
    auto& winner=records_[biggest]; if (!winner.inUse) winner.inUse=1;
    source.graph=biggest; winner.surfaces=Word(winner.surfaces+1);
    if (connections.size()>1) {
        // 앞서 flood한 연결의 번호도 다시 읽으며 패배 레코드의 reserved WORD를 보존한다.
        for (auto neighbor:connections) if (members_.at(neighbor).graph!=biggest) {
            const auto loser=members_.at(neighbor).graph; FloodImpl(neighbor,biggest); Free(loser);
        }
    }
}
// 삭제 준비는 원천을 dead로 표시한 시점에 호출한다. flood가 제거 원천을 다시 방문하지 않는다.
void Graph::Detach(std::uint16_t id,std::span<const std::uint16_t> connections,bool rebuild,std::uint8_t removedSurfaces) {
    DetachAt(id,members_.at(id).graph,connections,rebuild,removedSurfaces);
}
// 조회한 그래프 번호로 표를 고르되 dead 원천의 번호/상태/타입은 교체하지 않는다.
void Graph::DetachAt(std::uint16_t id,std::uint8_t graph,std::span<const std::uint16_t> connections,bool rebuild,std::uint8_t removedSurfaces) {
    CheckNumber(graph,true);
    if (!(members_.at(id).state&2) || (removedSurfaces!=1 && removedSurfaces!=9))
        throw std::invalid_argument("그래프 삭제 준비의 dead/감소 수 오류");
    auto next=*this; next.DetachImpl(id,graph,connections,rebuild,removedSurfaces);
    records_=next.records_; stack_=next.stack_; members_=std::move(next.members_);
}
// 원본은 남은 연결의 마지막 하나를 기존 그래프에 남긴다. 다른 번호의 이웃도 남은 수에 포함한다.
void Graph::DetachImpl(std::uint16_t id,std::uint8_t number,std::span<const std::uint16_t> connections,bool rebuild,std::uint8_t removedSurfaces) {
    if (number==kInvalid || !records_[number].inUse) return;
    if (connections.size()>=64) throw std::out_of_range("그래프 삭제 연결 목록 범위 오류");
    std::size_t remaining=connections.size(); bool keep=!rebuild;
    // 이전 flood가 바꾼 번호를 매번 다시 읽는다. 연결 그래프 번호를 미리 중복 제거하지 않는다.
    for (auto neighbor:connections) {
        if (members_.at(neighbor).graph==number) {
            if (!rebuild && remaining<2) {
                if (!(surfaces_.Object(id).flags2&8)) keep=true;
            } else FloodImpl(neighbor,Allocate());
        }
        --remaining;
    }
    // 고립된 원천은 감소하지 않는 원본 분기를 보존한다. 특수 타입은 9를 감소시킨다.
    if (!connections.empty()) records_[number].surfaces=Word(records_[number].surfaces-removedSurfaces);
    if (!keep) { records_[number].surfaces=0; records_[number].inUse=0; }
}
// 원본의 미사용 레코드/스택 흔적도 검사할 수 있도록 전체를 제공한다.
std::span<const GraphRecord> Graph::Records() const { return records_; }
// 전체 스택에서 사용하지 않는 DWORD도 보존한 채 돌려준다.
std::span<const std::uint32_t> Graph::FloodStack() const { return stack_; }
// 없는 번호는 std::map의 경계 검사로 거부한다.
std::uint8_t Graph::Number(std::uint16_t id) const { return members_.at(id).graph; }
// 그래프 연산은 입력 raw 상태를 바꾸지 않는다.
std::uint8_t Graph::State(std::uint16_t id) const { return members_.at(id).state; }
// 실제 그래프 수는 앞 251개만 센다. sentinel의 inUse 값은 포함하지 않는다.
std::size_t Graph::InUse() const {
    return static_cast<std::size_t>(std::count_if(records_.begin(),records_.begin()+kCount,[](const auto& r){return r.inUse!=0;}));
}
}
