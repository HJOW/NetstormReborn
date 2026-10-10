// 서버 건설 요청(Construction.cpp 00444760 / CD 004d17d0)의 검증·받침 확정·실패 정리 순서를 복원한다.
#pragma once
#include "o/RawConstructionClear.h"
#include "o/RawConstructionConfirm.h"
#include "o/RawCanonPlacementPipeline.h"

namespace netstorm::o {
// 요청이 읽는 전역이다. 공통 배치 모듈의 editor/localPlayer/패턴 번호와 같은 값을 공급한다.
struct ConstructionRequestState {
    bool editor{};                 // 00594fb8 / 00540a1c: 편집 모드에서는 요청 플레이어를 로컬 번호로 바꾼다.
    std::uint32_t localPlayer{};    // 00540c70 / 0050f6c8: BYTE로 줄이지 않고 그대로 사용한다.
    std::uint32_t displaceGenus{};  // 005325b4 / 0053fc28: 이 genus에 해당하면 기존 유닛을 밀어낸다.
    std::array<std::uint32_t,8> patternTypes{107,82,94,157,131,129,140,142}; // decoder와 island/noIsland 번호 전역.
};
// 요청 밖의 효과다. 공통 세 경계와 판본에 맞는 유닛 밀어내기 경계가 필수다.
struct ConstructionRequestHooks {
    // 0049b510 / 00445200: 원본 인자 그대로 전체 배치 가능 판정을 호출한다. mode는 항상 0이다.
    std::function<bool(const CanonPlacementQuery&)> mayPlace;
    // 00443800(10.78): 유닛을 밀어내고 실패하면 요청을 중단한다. 이 함수 전체의 실제 효과는 후속이다.
    std::function<bool(float,float,std::uint32_t,std::uint32_t)> displace;
    // 004d2450(CD): 플레이어 인자가 없는 void 호출이다. 반환 결과를 검사하지 않는다.
    std::function<void(float,float,std::uint32_t)> displaceCd;
    // 00444590 / 004d0f10: 받침 또는 요청 자체를 확정한다. 받침 확정의 반환값은 무시한다.
    std::function<bool(const ConstructionConfirmRequest&)> confirm;
    // 0046e6c0 / 0048f960: flags1 0x400 타입의 본 확정이 실패했을 때 (x,y,0x1000,player)로 정리한다.
    std::function<void(float,float,std::uint32_t,std::uint32_t)> cancel;
    // 원본에 없는 진단 지점이다. 각 decoder 칸의 탐색 범위만 관찰하며 비워 둘 수 있다.
    std::function<void(const SquidSearchArea&)> scan;
};
// 전체 서버 요청의 실행 계층이다. 로컬 예측/커서와 메시지 수신은 이 계층의 호출자가 담당한다.
class RawConstructionRequest {
public:
    // 원본 지도는 256×256이다. spot 표는 최소 이 크기여야 한다.
    static constexpr std::size_t kSpotCount=256*256;
    // 풀·해시·타입/프레임·spot·전역은 더 오래 살아야 한다. 필수 경계 누락은 첫 효과 전에 거부한다.
    RawConstructionRequest(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const std::uint8_t> spots,
        const ConstructionRequestState& state,ConstructionRequestHooks hooks);
    // 편집 소유자 보정 → MayPlace → 모든 칸의 abstract 충돌 검사 → 유닛 밀어내기 → 받침/본 확정 → 실패 정리.
    // 요청 타입·패턴 타입이 표 밖이거나 좌표가 유한하지 않으면 효과 없이 거부한다.
    bool Request(ConstructionConfirmRequest request) const;
    // 실제 모듈 어댑터가 같은 풀을 쓰는지 확인한다.
    const SidPool& Pool() const;
private:
    // float 발자국 사각형 안의 정수 칸에 spot bit 2가 하나도 없으면 true다. 큰 모서리는 올림하지 않는다.
    bool NoSurface(const PriestPlacementRect& rectangle) const;
    // 표면 조회는 +0.9999 뒤 절삭한다. 범위 밖 좌표에서는 예약 슬롯 0을 사용한다.
    Sid Surface(float x,float y) const;
    const SidPool& pool_;
    const SquidHash& hash_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    std::span<const std::uint8_t> spots_;
    const ConstructionRequestState& state_;
    ConstructionRequestHooks hooks_;
};
// 전체 MayPlace 파이프라인과 실제 서버 확정을 같은 풀로 연결한다. 밀어내기/실패 정리/진단은 받은 경계를 둔다.
ConstructionRequestHooks MakeConstructionRequestHooks(const SidPool& pool,RawCanonPlacementPipeline& placement,
    const RawConstructionConfirm& confirm,ConstructionRequestHooks hooks);
}
