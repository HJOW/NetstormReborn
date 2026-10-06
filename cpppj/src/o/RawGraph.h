// 실제 SID·프레임 코드·0단계 해시·spot을 정수 Graph 계산에 연결한다.
#pragma once
#include "o/Graph.h"
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
// 공간 쓰기 전 Pop이 검사할 최종 좌표·해시 단계·타입이다. 타입 포인터는 검사 동안만 사용한다.
struct RawGraphPop { float x{},y{}; int level{}; const RiftTypeRecord* type{}; };
class RawGraph {
public:
    // RiftType의 실제 FrameCode 표를 복사한다. 풀·해시·spot의 수명은 RawGraph보다 길어야 한다.
    RawGraph(SidPool& pool,SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const std::vector<FrameCode>> frames,
        std::span<const GraphRecord> records={},std::span<const std::uint32_t> stack={});
    // 새 좌표/void 해제/spot·해시 등록까지 예측하여 실패를 공간 쓰기 전에 검출한다.
    void ValidateAdd(Sid sid,const RawGraphPop* pop=nullptr) const;
    // 계산이 모두 성공한 뒤 관련 슬롯의 graph byte와 표/스택을 함께 반영한다.
    void Add(Sid sid);
    // 기존 raw 표면을 실제 탐색 순서로 flood하고 변경한 객체 수를 돌려준다.
    std::uint32_t Flood(Sid sid,std::uint8_t graph);
    // 기존 레코드의 reserved WORD와 전체 스택의 미사용 흔적도 노출한다.
    std::span<const GraphRecord> Records() const;
    // 다음 스냅샷 계산에서도 이 스택을 초기화하지 않는다.
    std::span<const std::uint32_t> FloodStack() const;
    // postPop/Pop의 공유 풀·지도 연결을 검사한다.
    const SidPool& Pool() const;
    // 서로 다른 지도에 Pop하고 Graph를 갱신하는 연결은 생성 시 거부한다.
    void ValidateSpace(const SquidHash& hash,std::span<const std::uint8_t> spots) const;
    // postPop과 표면 필터/발자국이 다른 타입 표를 연결하지 않는다.
    void ValidateTypes(std::span<const RiftTypeRecord> types) const;
private:
    // 예외가 생겨도 기존 raw 슬롯·표·스택을 보존하는 계산 결과다.
    struct Plan {
        std::array<GraphRecord,Graph::kTableSize> records;
        std::array<std::uint32_t,Graph::kFloodSize> stack;
        std::vector<std::pair<Sid,std::uint8_t>> numbers;
        std::uint32_t changed{};
    };
    // 각 호출에서 현재 raw 입력을 다시 읽는다. noGraph 리셋/프레임 변경을 캐시하지 않는다.
    Plan Calculate(Sid sid,bool add,std::uint8_t target,const RawGraphPop* pop) const;
    // 검사한 슬롯의 graph 필드 외에는 쓰지 않는다.
    void Commit(const Plan& plan);
    SidPool& pool_;
    SquidHash& hash_;
    std::span<const std::uint8_t> spots_;
    std::vector<RiftTypeRecord> types_;
    std::vector<std::vector<FrameCode>> frames_;
    std::array<GraphRecord,Graph::kTableSize> records_{};
    std::array<std::uint32_t,Graph::kFloodSize> stack_{};
};
}
