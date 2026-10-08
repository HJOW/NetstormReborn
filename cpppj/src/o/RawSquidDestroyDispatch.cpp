// 004220f0 ↔ CD 00449820. 가상 다리 삭제의 단락 조건과 공통 몸체의 직접 호출을 구별한다.
#include "o/RawSquidDestroyDispatch.h"
#include "o/RawBridgeEvents.h"
#include "o/RawGraph.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 패치 프레임과 두 판본 가상 표는 little endian DWORD다.
std::uint32_t Dword(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 주변 필드를 섞지 않고 네 바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
}
// 삭제 몸체와 프레임 탐색기·Graph가 같은 풀/공간을 참조하는지 구성 시 확인한다.
RawSquidDestroyDispatch::RawSquidDestroyDispatch(RawSquidDestroy& base,const RawSquidNeighbors& neighbors,
    const BridgeDecayMode& mode,const RawGraph* graph):base_(base),neighbors_(neighbors),mode_(mode),graph_(graph) {
    if (&base.Pool()!=&neighbors.Pool()) throw std::invalid_argument("가상 삭제/프레임 SID 풀이 다릅니다");
    if (graph) {
        if (&graph->Pool()!=&base.Pool()) throw std::invalid_argument("가상 삭제/Graph SID 풀이 다릅니다");
        base.ValidateGraph(*graph);graph->ValidateTypes(neighbors.Types());
    }
}
// 타입 번호 대신 실제 raw 가상 표로 재정의를 고른다. 거부 시 dead·장부·표시·프로세스를 바꾸지 않는다.
bool RawSquidDestroyDispatch::Destroy(Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) const {
    const auto& pool=base_.Pool();const auto raw=pool.Slot(sid);
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    if (Dword(raw,0)==(patch ? kPatchBridgeVtable : kCdBridgeVtable)) {
        const auto extra=raw[patch ? 40 : 35];
        // editor/권한/abstract/buried 조건이 통과할 때만 그래프 번호와 현재 프레임을 읽는다.
        if (!mode_.editor && mode_.authority && (extra&9)==0) {
            std::int16_t surfaces=0;
            if (graph_) {
                const auto number=raw[patch ? 30 : 28];
                // 원본은 254 조회의 NULL을 역참조한다. 호스트에서는 쓰기 전 예외로 표시한다.
                if (number==Graph::kInvalid) throw std::logic_error("가상 다리 삭제의 무효 Graph 번호");
                if (number>=graph_->Records().size()) throw std::out_of_range("가상 다리 삭제의 Graph 표 범위 오류");
                surfaces=graph_->Records()[number].surfaces;
            }
            if (surfaces>=kBridgeMinimumGraphSurfaces) {
                const auto frame=patch ? std::bit_cast<std::int32_t>(Dword(raw,36)) : static_cast<std::int32_t>(raw[34]);
                const auto codes=neighbors_.Frames(raw[10]).Codes();
                if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("가상 다리 삭제 프레임 범위 오류");
                if (!Bridge::DestroyProceeds(mode_,extra,surfaces,codes[static_cast<std::size_t>(frame)].flags)) return false;
            }
        }
    }
    base_.Destroy(sid,flags,hooks);return true;
}
}
