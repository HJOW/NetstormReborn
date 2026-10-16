// 서로 다른 효과가 하나의 원본 표시 목록·토큰·수집/분배 경계를 공유하게 한다.
#pragma once
#include "o/RawRenderCollection.h"
#include "o/SquidProcess.h"
#include <unordered_map>

namespace netstorm::o {
// PATCH00487960: 영역 각 변을 먼저 정수화한 뒤 카메라/여유를 적용하는 Form 삭제 통지다.
// 일반 프레임의 dirty 계산과 달리 초기 정수화 및 중간 float 저장 경계를 보존한다.
std::array<std::int32_t,4> RenderProcessRemovalBounds(const RenderProcessFields& fields,float cameraX,float cameraY);
// Kernel 소유권은 각 효과의 Host가 유지한다. 이 저장소는 실제 등록/출력 경계만 공유한다.
class RenderProcessSystem {
public:
    // pool/host/state는 이 저장소보다 오래 살아야 한다. 효과 시스템들은 이 저장소보다 먼저 해제한다.
    RenderProcessSystem(SidPool& pool,SquidProcessHost& host,ConstructionVisualState& state,RenderRegistryHooks hooks);
    RenderProcessSystem(const RenderProcessSystem&)=delete; // 등록 콜백의 고정 주소를 유지한다.
    RenderProcessSystem& operator=(const RenderProcessSystem&)=delete; // 다른 월드의 목록으로 복사하지 않는다.
    // 원본 등록 전에 비영 토큰과 생성 중 조회 항목을 확보한다. attach는 실제 Host 부착을 수행해야 한다.
    std::uint32_t Track(RenderProcessFields& fields,
        std::function<void(RenderProcessFields&,std::uint32_t,Sid,std::uint32_t)> attach,std::function<void()> draw);
    // 파생 초기화가 끝난 뒤에만 수집/Draw를 허용한다.
    void Ready(std::uint32_t token);
    // 효과의 실제 소멸자가 버퍼/원본 등록을 정리한 뒤 조회 항목을 지운다. 미등록 토큰은 무시한다.
    void Forget(std::uint32_t token);
    // 같은 원본 목록의 슬롯 순서로 초기화 완료 효과만 출력한다.
    void Draw() const;
    // 공통 kind3 분배에서 사용하는 현재 토큰이다. 수집 후 삭제된 항목은 생략한다.
    void DrawToken(std::uint32_t token) const;
    // 기존 원본 collector로 모든 효과를 한 번에 수집한다. 종류별 목록을 합치지 않는다.
    std::int32_t Collect(RenderWorldRect view,std::span<RenderCollectionEntry> output,std::int32_t limit) const;
    // 생성/삭제의 원본 몸체가 사용할 단일 registry다.
    RawRenderRegistry& Registry();
    // 원본 구멍·40칸 확장을 그대로 관찰한다. 확장 뒤에는 이전 span이 무효다.
    std::span<const std::uint32_t> Slots() const;
    // 생성 중인 항목도 포함하는 현재 조회 수다.
    std::size_t Size() const;
    // 초기화 완료 토큰의 현재 등록 필드다. 삭제 뒤 이 포인터를 보관/사용하지 않는다.
    const RenderProcessFields* Lookup(std::uint32_t token) const;
    // 효과가 같은 월드·Host·좌표 배율을 사용하는지 검증할 참조다.
    SidPool& Pool() const;
    SquidProcessHost& Host() const;
    ConstructionVisualState& State() const;
private:
    // 원본 registry의 attach만 현재 토큰에 등록한 실제 Host 콜백으로 바꾼다.
    RenderRegistryHooks Hooks(RenderRegistryHooks hooks);
    // 객체 수명은 호출자가 유지하며 ready 전에는 fields의 파생 내용을 읽지 않는다.
    struct Binding {
        RenderProcessFields* fields{}; // 해당 효과가 소유하는 실제 등록 필드다.
        std::function<void(RenderProcessFields&,std::uint32_t,Sid,std::uint32_t)> attach; // 생성 중인 객체의 Host 부착이다.
        std::function<void()> draw; // 현재 카메라/부모를 읽는 실제 파생 Draw다.
        bool ready{}; // 파생 생성 완료 여부다.
    };
    SidPool& pool_; // 모든 표시가 읽는 단일 raw 풀이다.
    SquidProcessHost& host_; // 실제 client Form/Kernel의 공통 호스트다.
    ConstructionVisualState& state_; // 원본 수집기가 읽는 현재16/11 배율이다.
    RawRenderRegistry registry_; // 효과 종류와 무관한 원본 단일 표시 목록이다.
    std::unordered_map<std::uint32_t,Binding> bindings_; // 비영 토큰에서 현재 살아 있는 효과를 조회한다.
    std::uint32_t nextToken_{1}; // 모든 효과가 공유하는 식별자이며 감김은 거부한다.
};
}
