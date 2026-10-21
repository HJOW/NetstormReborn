// 원본 AI의 생산 조각 소비·대기 예약·실제 배치004106e0과 비용00410280/004102b0을 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawContainedCreate.h"
#include "o/RawConstructionConfirm.h"
#include "o/RawConstructionLocal.h"

namespace netstorm::o {
// 같은 원본 권한/건설 사제 이동/시각 저장소를 빌린다.
struct AiDeploymentGlobals {
    const bool& authority; //00540bc4:서버 확정과 로컬 예측 분기를 선택한다.
    bool& moveLocalBuilder; //00544d10:실제 배치 요청 동안1을 쓰고 정상 반환 뒤0을 쓴다.
    const double& now; //0055b4d0:배치 뒤 시설 종류별 QWORD 기한에 더한다.
};
// 기존 실제 생산 조회·수명·타이머·건설 몸체와 진단/데모 장치를 연결한다.
struct AiDeploymentHooks {
    std::function<Sid(std::uint32_t,std::uint32_t)> product; //004103c0의 같은 본체 owner→Player 기술부모→mask8 조회다.
    std::function<Sid(std::uint32_t)> technology; //0048fc90의 현재 Player 기술부모다.
    std::function<void(Sid,std::uint32_t)> destroy; //가상+10의 실제 조각 삭제다.
    std::function<bool()> reserveRegular; //원본 new(0x28)의 성공 여부이며 실패해도 나머지 배치는 계속한다.
    std::function<void(Sid,float)> addRegular; //예약된 메모리에 event0/현재rate를 구성하고 같은 Kernel에 부착한다.
    std::function<void(const ConstructionConfirmRequest&)> request; //00444760의10인자 실제 서버 요청이며 반환값은 원본이 무시한다.
    std::function<void(ConstructionLocalRequest)> localPlace; //004433b0의 실제 로컬 예측/서버 송신이다.
    std::function<bool()> demo; //00463b50의 현재 데모 여부다.
    std::function<void(std::string_view)> assertion; //원본 진단 뒤 계속하는 장치 효과다.
};
// 실제 내용물과 본체 지갑만 변경하며 별도 SP 차감이나 임시 전략을 만들지 않는다.
class RawAiDeployment {
public:
    RawAiDeployment(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,
        const RawContainedCreate& create,AiDeploymentGlobals globals,AiDeploymentHooks hooks); //같은 PATCH Pool/TYPE/내용물 생성 연결을 확인한다.
    bool CanAfford(std::uint32_t token,std::uint32_t type) const; //00410280:원본ftol(cost)의 signed 하위DWORD와 현재 지갑을 비교한다.
    void Charge(std::uint32_t token,std::uint32_t type) const; //004102b0:ftol(-cost) 감김 가산 뒤 현재 데모 하한을 적용한다.
    void Place(std::uint32_t token,std::uint32_t type,float x,float y,std::uint32_t argument,std::uint32_t direction) const; //004106e0 전체의 생산/대기/비용/배치/기한 순서다.
    const SidPool& Pool() const; //실제 월드 조립 시 같은 풀인지 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재0xb08 본체를 다시 빌린다.
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; //현재 본체 DWORD를 읽는다.
    void Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const; //선택된 DWORD만 쓴다.
    const RiftTypeRecord& Type(std::uint32_t type) const; //현재 원본 TYPE를 범위 검사 뒤 조회한다.
    Sid Technology(std::uint32_t token) const; //현재 owner로 실제 기술부모를 다시 조회한다.
    void Deadline(std::uint32_t token,std::size_t offset) const; //현재 QWORD now+20초를 지정 두DWORD에 쓴다.
    RawAiBrainMemory& memory_; //기존 본체/목록/지갑과 같은 유일한 저장소다.
    const SidPool& pool_; //생산 조각과 건설이 공유하는 실제 풀이다.
    std::span<const RiftTypeRecord> types_; //매 원문 위치에서 다시 읽는 현재 TYPE 비용/flags2다.
    const RawContainedCreate& create_; //실제 생산 내용물/예측 조각 생성과 부착 몸체다.
    AiDeploymentGlobals globals_; //현재 권한/이동 플래그/시각의 참조다.
    AiDeploymentHooks hooks_; //선택된 실제 원본 하위 효과다.
};
}
