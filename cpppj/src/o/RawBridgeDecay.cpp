//논리 스냅샷을 일괄 반영하지 않고 원문의 직접 쓰기와 재진입 효과를 같은 슬롯에 적용한다.
#include "o/RawBridgeDecay.h"
#include "o/RawAuthorityState.h"
#include "o/RawCanonPlacementSurrounding.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //원문 PATCH의 한 칸/수명 몸체이며 CD 인라인 경로는 별도다.
namespace { //외부 효과를 빈 성공으로 대신하지 않는다.
//선택된 현재 효과가 빠졌으면 앞선 WORD/프레임 쓰기 상태에서 오류를 전달한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("다리 붕괴의 실제 효과 미연결");return value; }
//비정렬 DWORD/WORD를 원문의 little endian 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(raw[offset+byte])<<(byte*8);return value; //필드 바이트만 조립한다.
}
//직접 프레임 쓰기는 별도 공간 재등록을 하지 않는다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    for (std::size_t byte=0;byte<width;++byte) raw[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); //원문 지정 폭의 낮은 바이트부터 쓴다.
}
}
//초기화/구성은 저장된 수명·스캔·Graph를 수정하지 않는다.
RawBridgeDecay::RawBridgeDecay(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    std::span<const std::uint16_t> surface,std::span<const std::uint8_t> spots,std::span<const GraphRecord> graphs,
    const BridgeDecayMode& mode,RawBridgeDecayHooks hooks,const RawAuthorityState* authority)
    :pool_(pool),types_(types),frames_(frames),surface_(surface),spots_(spots),graphs_(graphs),mode_(mode),hooks_(std::move(hooks)),authority_(authority) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=types.size() || surface.size()!=65536 || spots.size()!=surface.size() || graphs.size()!=255)
        throw std::invalid_argument("다리 붕괴의 PATCH 풀/현재 표면/Graph 표 오류");
}
//원문의 물리 주소를 매번 새로 빌려 효과 중 풀 재확보 뒤의 오래된 span을 쓰지 않는다.
std::span<std::uint8_t> RawBridgeDecay::Bytes(Sid sid) const { return std::span(pool_.bytes_).subspan(pool_.Offset(sid),pool_.Layout().stride); }
//잘못된 프레임은 해당 칸의 원문 예외 경계로 올리며 임의 프레임으로 대신하지 않는다.
FrameCode RawBridgeDecay::Code(Sid sid) const {
    const auto raw=pool_.Slot(sid);if (raw[10]>=frames_.size()) throw std::out_of_range("다리 붕괴의 현재 TYPE 범위");
    const auto codes=frames_[raw[10]].Codes();const auto number=Read(raw,36);if (number>=codes.size()) throw std::out_of_range("다리 붕괴의 현재 프레임 범위");return codes[number];
}
//생성된 원천 기하만 고정하고 후보/지도는 같은 이미 복원된 flag8 Next에서 읽는다.
std::vector<Sid> RawBridgeDecay::Neighbors(Sid sid) const {
    std::vector<Sid> result;RawCanonSurfaceWalk walk(pool_,types_,frames_,surface_,spots_,sid);
    for (auto neighbor=walk.Current();neighbor.value;neighbor=walk.Next()) result.push_back(neighbor); //원문 y/x·중복 제거 순서를 보존한다.
    return result;
}
//원문은 방문 집합 대신 부모 하나만 제외한다. 고리/과도한 재귀는 호스트의 안전 중단으로 표시한다.
BridgeDecayWalk RawBridgeDecay::Collect(Sid root) const {
    BridgeDecayWalk result;result.visited.push_back(root.value);std::vector<Sid> active;
    const auto visit=[&](auto&& self,Sid sid,int depth,Sid parent)->void { //원문004218b0의 각 재귀 호출마다 현재 이웃 목록을 읽는다.
        if (depth>=256 || std::find(active.begin(),active.end(),sid)!=active.end()) { result.complete=false;result.canDecay=false;return; }
        active.push_back(sid);
        for (const auto neighbor:Neighbors(sid)) { //다리/섬만 처리하며 부모 외의 중복은 원문처럼 목록에 남긴다.
            const auto raw=pool_.Slot(neighbor);const auto flags=types_[raw[10]].flags2;const bool bridge=(flags&TypeFlag2::kBridge)!=0,island=(flags&TypeFlag2::kIsland)!=0;
            if ((!bridge && !island) || neighbor==parent) continue;
            if (bridge && result.visited.size()<100) result.visited.push_back(neighbor.value);
            if (Code(neighbor).side<'J' && !island) {
                if (depth==0 && Neighbors(neighbor).size()<3) result.shortJunction=true;
                self(self,neighbor,depth+1,sid);if (!result.complete) break;
            } else if (depth==0 || (depth==1 && result.shortJunction)) { result.canDecay=true;std::erase(result.visited,neighbor.value); }
            else { if (!bridge || Neighbors(neighbor).size()<2) result.canDecay=true;else std::erase(result.visited,neighbor.value); }
        }
        active.pop_back();
    };
    visit(visit,root,0,Sid{});return result;
}
//수명은 signed DWORD에서 빼고 음수면0으로 제한한다. 제거 분기는 WORD를 쓰지 않는다.
void RawBridgeDecay::ApplyLife(Sid sid,std::int32_t reduction) const {
    const auto previous=static_cast<std::int32_t>((Read(pool_.Slot(sid),12,2)&0x78U)>>3);
    auto remaining=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(previous)-std::bit_cast<std::uint32_t>(reduction));if (remaining<0) remaining=0;
    if (!remaining && (authority_ ? authority_->server : mode_.server)) { const auto side=Code(sid).side;Need(hooks_.destroy)(sid,side=='J' || side=='K' ? kBridgePlankRemoval : 0U);return; }
    auto raw=Bytes(sid);Put(raw,12,(Read(raw,12,2)&~0x78U)|((static_cast<std::uint32_t>(remaining)*8U)&0x78U),2);Need(hooks_.notifySurface)(sid);
    if (remaining<kBridgeCrackLife && previous>=kBridgeCrackLife) {
        const auto current=pool_.Slot(sid);if (current[10]>=frames_.size()) throw std::out_of_range("다리 수명 통지 뒤의 현재 TYPE 범위");
        const auto frame=Bridge::CrackedFrame(frames_[current[10]],std::bit_cast<std::int32_t>(Read(current,36)),false);
        if (frame!=-1) { Put(Bytes(sid),36,static_cast<std::uint32_t>(frame));Need(hooks_.refresh)(sid); }
    } else if (mode_.debugRedraw) Need(hooks_.refresh)(sid);
    if (remaining<=kBridgeCarrierLife) Need(hooks_.carriers)(sid);
}
//각 방문의 삭제가 바꾼 dead/WORD는 다음 방문에서 다시 읽으며 전체를 사본으로 되돌리지 않는다.
void RawBridgeDecay::Cell(Sid root) const {
    const auto graph=pool_.Slot(root)[30];if (graph==Graph::kInvalid || graph>=graphs_.size()) throw std::out_of_range("다리 붕괴의 무효 Graph 원문 예외");
    if (graphs_[graph].surfaces<kBridgeMinimumGraphSurfaces) { Need(hooks_.destroy)(root,0);return; }
    const auto code=Code(root);if (code.flags&kBridgeHardFrame) return;
    const auto neighbors=Neighbors(root);if (neighbors.empty()) { ApplyLife(root,1);return; }
    std::vector<int> listed;for (std::size_t index=0;index<neighbors.size() && index<10;++index) listed.push_back(neighbors[index].value); //원문의 첫10개 목록만 열린 방향 판정에 쓴다.
    const auto raw=pool_.Slot(root);if (Bridge::OpenDirection(static_cast<char>(code.side),std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),surface_,listed)==-1) return;
    const auto walk=Collect(root);if (!walk.complete) throw std::logic_error("원문 다리 재귀 고리의 호스트 안전 중단");if (!walk.canDecay) return;
    std::int32_t lowest=std::numeric_limits<std::int32_t>::max();
    for (const auto number:walk.visited) { const auto life=static_cast<std::int32_t>((Read(pool_.Slot(Sid{number}),12,2)&0x78U)>>3);if (life && life<=lowest) lowest=life; } //방문 목록의 현재 비영 수명만 최소값에 반영한다.
    const bool none=lowest==std::numeric_limits<std::int32_t>::max();if (!none && lowest>7) lowest=7;const auto target=none ? 7 : lowest-1;
    for (const auto number:walk.visited) { const Sid sid{number};const auto current=pool_.Slot(sid);if (!(current[11]&2U)) ApplyLife(sid,static_cast<std::int32_t>((Read(current,12,2)&0x78U)>>3)-target); } //앞선 실제 삭제/반납의 결과를 이후 방문에서 다시 읽는다.
    if (target<kBridgeCrackLife && lowest>=kBridgeCrackLife) Need(hooks_.crackSound)(root);
}
}
