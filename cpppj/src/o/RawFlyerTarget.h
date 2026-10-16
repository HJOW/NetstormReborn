// 원본 비행체의 표적 검색·의존 경쟁 수·포위 위치를 같은 현재 월드에서 계산한다.
#pragma once
#include "o/RawFlyerProcess.h"
#include "o/RawSquidFinder.h"

namespace netstorm::o {
// 원본이 매번 읽는 권한·진단·시각과 내장 타입 경계다.
struct FlyerTargetGlobals {
    const bool& server; // 00540bc0의 현재 표적 검색 권한이다.
    const bool& checking; // 005e4794의 현재 진단 선택이다.
    const double& now; // 0055b4d0의 단일 게임 시각이다.
    const std::uint32_t& sunType; // 0054117c의 훨리기그 타입73이다.
    const std::uint32_t& processEnd; // 0054116c의 프로세스 타입 상한63이다.
    const std::uint32_t& dependType; // 00541074의 의존 Form 타입1이다.
};
// 큰 기존 월드 소비자만 경계로 연결한다. 산술과 선택 규칙은 본체에서 실행한다.
struct FlyerTargetHooks {
    std::function<void(std::uint32_t)> assertion; // 원본 Flyer.cpp 또는 Baseprocess.cpp 진단 줄이다.
    std::function<bool(Sid)> checkForms; // 같은 실제004ad730의 종속 체인 검사다.
    std::function<std::array<float,2>(Sid)> offset; // 현재 부모 가상+cc의 이동 위치 보정이다.
    std::function<Sid(SquidSearchArea,std::uint32_t)> begin; // 실제 일반 finder의 정수 사각형/flags와 첫 후보다.
    std::function<Sid()> next; // 같은 커서의 실제004b1810이다.
    std::function<bool(Sid,Sid)> mayTarget; // 현재 부모의8DWORD 문맥과 후보를 실제0046af80에 전달한다.
    std::function<Sid(Sid)> watcher; // 실제004ae510의 상대 의존 Form 부모다.
    std::function<Sid(Sid)> processParent; // 실제004ae2d0의 현재 inner+8 부모다.
    std::function<double(FlyerProcessFields&)> returnDelay; // 현재 outer+c의 실제 수명/복귀 지연이다.
    std::function<void(Sid,std::uint32_t)> command; // 최초 훨리기그 표적 결정 시 명령2를 실행한다.
};
// 원본10.78의 검색·경쟁·포위 규칙을 같은 raw 필드와 의존 체인으로 복원한다.
class RawFlyerTarget {
public:
    RawFlyerTarget(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const float> ranges,
        FlyerTargetGlobals globals,FlyerTargetHooks hooks); // 현재188TYPE·사거리·장기 저장소를 확인한다.
    void Find(FlyerProcessFields& fields) const; // 0045c4b0의 출발점·필터·거리·첫 동률·복귀 기한을 복원한다.
    std::int32_t CloserCount(Sid target,Sid flyer) const; // 0045b840의 이미 더 가까운 다른 비행체 수다.
    std::int32_t Capacity(Sid target) const; // 0045b9c0의1×1 비사망 표적1칸·그 밖3칸이다.
    std::int32_t SlotIndex(Sid target,Sid flyer) const; // 0045ba10의 자기 의존 Form보다 작은 번호의 비행체 수다.
    std::array<float,2> Scatter(Sid target,Sid flyer) const; // 0045bb80의 실제 세 포위 오프셋을 선택한다.
    const SidPool& Pool() const; // 다른 월드의 검색/의존 저장소 혼합을 거부한다.
private:
    std::array<float,2> Center(Sid sid) const; // 현재 raw 좌표에서 실제 TYPE 발자국 절반을 뺀다.
    Sid Parent(FlyerProcessFields& fields) const; // 실제 Parent getter의 head 진단을 포함한다.
    SidPool& pool_; // 부모·표적·모든 의존 Form의 단일 raw 풀이다.
    std::span<const RiftTypeRecord> types_; // 현재 불변 TYPE의 genus·발자국이다.
    std::vector<float> ranges_; // 같은 번호의 실제 TYPE+44 불변 사거리를 보관한다.
    FlyerTargetGlobals globals_; // 효과 뒤 다시 읽는 현재 전역 참조다.
    FlyerTargetHooks hooks_; // 기존 동일 월드의 실제 큰 소비자다.
};
}
