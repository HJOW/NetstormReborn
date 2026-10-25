// 실제 SID·프레임 코드·4단계 해시·spot을 정수 Graph/영역 계산에 연결한다.
#pragma once
#include "o/Graph.h"
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
// 공간 쓰기 전 Pop이 검사할 최종 좌표·해시 단계·타입이다. 타입 포인터는 검사 동안만 사용한다.
struct RawGraphPop { /* x: X 좌표. */ /* y: Y 좌표. */ float x{},y{}; /* level: 공간 해시에서 조회할 높이 단계. */ int level{}; /* type: 객체 종류를 구별하는 타입 자료. */ const RiftTypeRecord* type{}; };
// World 실행기보다 오래 보관할 두 실제 배열이다. 원문의 별도 heap 두 개를 호스트의 한 연속 Native 객체로 가장하지 않는다.
struct RawGraphStorage {
    std::array<GraphRecord,Graph::kTableSize> records{}; //00562c18의255행6BYTE이며 reserved와 틈3행을 포함한다.
    std::array<std::uint32_t,Graph::kFloodSize> stack{}; //00562c20의4096DWORD이며 이전 flood의 비활성 흔적도 남긴다.
};
//00462b20의 지정WORD/BYTE만 쓴다. 시작00462eb0의 최초 할당/전체 초기화나 stack의 lazy 확보는 포함하지 않는다.
void ResetRawGraphTable(RawGraphStorage& storage);
// 실제 SID·프레임 코드·4단계 해시·spot을 정수 Graph/영역 계산에 연결한다.
class RawGraph {
public:
    // RiftType의 실제 FrameCode 표를 복사한다. 풀·해시·spot의 수명은 RawGraph보다 길어야 한다.
    // FullPool은 Add/Detach/Pop에서 전체 자산 풀을 검사한 뒤 소진 복구를 허용하는 계약이다.
    RawGraph(SidPool& pool,SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const std::vector<FrameCode>> frames,
        std::span<const GraphRecord> records={},std::span<const std::uint32_t> stack={},GraphRecovery recovery=GraphRecovery::Reject);
    // 명시 장기 물리만 빌리며 생성/소멸에서 기록·stack을 쓰지 않는다. 타입/코드 snapshot과 recovery 정책은 이 실행기가 소유한다.
    RawGraph(SidPool& pool,SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const std::vector<FrameCode>> frames,
        RawGraphStorage& storage,GraphRecovery recovery=GraphRecovery::Reject);
    RawGraph(const RawGraph&)=delete;RawGraph& operator=(const RawGraph&)=delete; // 독립 사본은 records/stack 입력 생성자로 만들며 내부 참조를 복제하지 않는다.
    RawGraph(RawGraph&&)=delete;RawGraph& operator=(RawGraph&&)=delete; // 현재 실행기가 빌린 실제 물리의 주소를 이동하지 않는다.
    // 새 좌표/void 해제/spot·해시 등록까지 예측하여 실패를 공간 쓰기 전에 검출한다.
    void ValidateAdd(Sid sid,const RawGraphPop* pop=nullptr) const;
    // 영역 통지→표면 Add 순서를 한 복사본에서 시험하여 Pop 전에 모든 실패를 검출한다.
    void ValidatePostPop(Sid sid,bool invalidate,bool add,const RawGraphPop* pop=nullptr) const;
    // 00462d40 ↔ CD 0045c210: 4단계 해시의 발자국 교차 후보 중 내부 spot 표면을 무효화한다.
    void InvalidateRegion(Sid sid);
    // 공통 postPop의 영역 통지와 표면 Add를 성공한 최종 결과로 함께 반영한다.
    void PostPop(Sid sid,bool invalidate,bool add);
    // 계산이 모두 성공한 뒤 관련 슬롯의 graph byte와 표/스택을 함께 반영한다.
    void Add(Sid sid);
    // 원천의 위치/타입/프레임과 0단계 머리 SID의 그래프 번호를 구별한다. 빈 머리는 자연 반환한다.
    // dead 원천의 이웃 분할·1/특수 9 감소를 계산하며 Unpop/반납과 별도로 호출한다.
    // sourceType은 삭제 위치 정보만 치환한다. raw 타입과 소진 복구의 전체 풀 입력은 보존한다.
    void Detach(Sid sid,bool rebuild=false,std::uint8_t removedSurfaces=1,std::optional<std::uint8_t> sourceType={});
    // 그래프 소진/손상 입력에서 공간 해제 전에 기존 번호/표/스택을 보존한다.
    void ValidateDetach(Sid sid,bool rebuild=false,std::uint8_t removedSurfaces=1,std::optional<std::uint8_t> sourceType={}) const;
    // 직접 Free는 좌표/이웃/raw 번호를 바꾸지 않고 지정 레코드의 앞 두 WORD만 지운다.
    void ValidateFree(std::uint8_t graph) const;
    void Free(std::uint8_t graph);
    // 특수 삭제 분기의 원본 프레임 방향 문자를 읽는다. raw 타입/프레임 표를 캐시하지 않는다.
    FrameCode Frame(Sid sid) const;
    // 공통 postDestroy의 발자국→일반 finder→표면별 Add를 같은 사본에서 연속 계산한다.
    void ValidatePostDestroy(Sid sid) const;
    void PostDestroy(Sid sid);
    // 기존 raw 표면을 실제 탐색 순서로 flood하고 변경한 객체 수를 돌려준다.
    std::uint32_t Flood(Sid sid,std::uint8_t graph);
    // 전체 raw 풀에서 무효 번호 수집 또는 전체 초기화 재구성을 원자적으로 반영한다.
    void Rebuild(bool resetAll);
    // 원본00462b20의 월드 초기 표를 만든다. Graph0 예약·reserved LOW BYTE·sentinel만 초기화하고 풀/스택은 보존한다.
    void ResetTable();
    // 빈 번호가 있으면 표만 갱신하고, 소진되면 전체 raw 풀 재구성 뒤 번호를 확보한다.
    std::uint8_t Allocate();
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
    // 독립 입력은 지역 물리에 복사하고 명시 shared 입력은 그대로 빌리는 공통 생성 경계다.
    RawGraph(SidPool& pool,SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const std::vector<FrameCode>> frames,
        RawGraphStorage* shared,std::span<const GraphRecord> records,std::span<const std::uint32_t> stack,GraphRecovery recovery);
    // 예외가 생겨도 기존 raw 슬롯·표·스택을 보존하는 계산 결과다.
    struct Plan {
        // records: 기록.
        std::array<GraphRecord,Graph::kTableSize> records;
        // stack: 스택.
        std::array<std::uint32_t,Graph::kFloodSize> stack;
        // numbers: 번호.
        std::vector<std::pair<Sid,std::uint8_t>> numbers;
        // changed: 상태 변경으로 후속 갱신이 필요한지.
        std::uint32_t changed{};
    };
    // Add/Flood 외에 영역 단독·영역 후 Add를 구별한다. 원본 순서를 한 계획에 보존한다.
    enum class Operation { Add,Flood,Region,RegionAdd,Detach,Rebuild,Allocate };
    // 각 호출에서 현재 raw 입력을 다시 읽는다. noGraph 리셋/프레임 변경을 캐시하지 않는다.
    Plan Calculate(Sid sid,Operation operation,std::uint8_t target,const RawGraphPop* pop,std::optional<std::uint8_t> sourceType={}) const;
    // 공간은 읽기 전용으로 공유하고 SID/그래프 사본에서 순차 Add의 번호/스택 변화를 이어 간다.
    Plan CalculatePostDestroy(Sid sid) const;
    // 원본 일반 탐색기의 단계/행/열/next 순서와 한 칸 넓은 끝 버킷을 유지한다.
    void Region(Sid sid,const RawGraphPop* pop,std::span<const std::uint8_t> spots,Plan& plan) const;
    // 삭제의 일반 finder는 네 단계/체인 순서와 가로·세로 경계 접촉 교차 XOR를 사용한다.
    // sourceFrameValid가 false이면 원본 마스크가 방향 코드를 덮어쓰는 연결만 해석한다.
    std::vector<std::uint16_t> DetachConnections(const SurfaceObject& source,const SurfaceFinder& finder,bool sourceFrameValid) const;
    // 검사한 슬롯의 graph 필드 외에는 쓰지 않는다.
    void Commit(const Plan& plan);
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // hash_: 좌표별 객체 후보를 찾는 공간 해시.
    SquidHash& hash_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::vector<std::vector<FrameCode>> frames_;
    RawGraphStorage localStorage_; // 기존 독립 snapshot과 CalculatePostDestroy의 임시 next만 사용하는 실제 사본이다.
    RawGraphStorage& storage_; // 명시 공유 입력 또는 위 지역 물리를 한 번 정하고 실행기보다 오래 유지한다.
    std::array<GraphRecord,Graph::kTableSize>& records_; // 모든 기존 계산/Commit이 같은 현재 기록을 읽고 쓴다.
    std::array<std::uint32_t,Graph::kFloodSize>& stack_; // 현재 기록과 같은 owner의 flood 흔적이다.
    // 전체 원본 자산 풀 계약을 선택한 경우 Add/Detach/Pop의 내부 번호 소진도 복구한다.
    GraphRecovery recovery_{GraphRecovery::Reject};
};
}
