// Build.cpp의 건설 표시 생성·버퍼 준비·좌표 동기화·초기 입자 선택을 복원한다.
#pragma once
#include "o/SidPool.h"
#include "o/SpStore.h"
#include <array>
#include <functional>
#include <vector>

namespace netstorm::o {
struct ConstructionProcessHooks;
// 현재 SHP 프레임을 캡처한 팔레트 픽셀이다. 0xff는 투명이며 hotspot은 화면 픽셀 기준이다.
struct ConstructionVisualImage {
    std::int32_t width{},height{},hotspotX{},hotspotY{}; // 캡처 경계가 공급한 크기와 기준점이다.
    std::vector<std::uint8_t> pixels; // width*height개 픽셀이다. 입자 선택이 원본 색을 0xff로 바꾼다.
};
// 원본 12바이트 입자 가운데 의미가 있는 값이다. 목적지는 출력 버퍼 내의 픽셀 번호로 표현한다.
struct ConstructionVisualParticle {
    std::uint8_t active{},color{}; // +0/+1: 활성 표식과 완성 뒤 되돌릴 원래 색이다.
    float life{}; // +4: 실제 게임 난수로 선택한 수명이다.
    std::uint32_t pixel{}; // +8: 원본 포인터 대신 출력 버퍼 내의 오프셋이다.
};
// 0x88바이트 건설 표시 객체의 복원한 필드다. 미초기화 영역은 가상의 의미로 채우지 않는다.
struct ConstructionVisualFields {
    Sid parent{}; // +8: 표시할 부모다. RenderProcess 부착 경계가 설정/변경할 수 있다.
    bool instant{}; // 파생 가상 표: 즉시 건설은 2~9, 일반 건설은 1.3~2의 수명 범위다.
    std::int32_t zOrder{}; // +0x20: 부모의 zOrder BYTE를 signed char로 확장한 값이다.
    float left{},top{},right{},bottom{}; // +0xc..+0x18: 표시 갱신 사각형이다. 동기화 때 너비/높이 0으로 맞춘다.
    double lastTime{},elapsed{}; // +0x30/+0x38: 프레임 시각/누적 시각이다. 생성자는 둘 다 0으로 둔다.
    std::uint32_t palette{}; // +0x48: 공유 입자 색 표의 호스트 항목이다.
    ConstructionVisualImage image; // +0x4c/+0x54..+0x60: 캡처 픽셀·크기·hotspot이다.
    std::vector<std::uint8_t> output; // +0x50: 투명으로 채운 표시 버퍼다.
    std::int32_t lastRow{}; // +0x64: (height-1)*width다.
    float minimum{},maximum{}; // +0x6c/+0x70: 입자 수명 범위다.
    std::uint32_t selected{},opaque{},active{}; // +0x74/+0x78/+0x80: 누적 선택·불투명 픽셀·활성 입자 수다.
    bool firstDraw{}; // +0x7c: 첫 RunFrame의 시각 helper 초기화 여부다. 생성 때 거짓이다.
    std::uint32_t capacity{},total{}; // +0x24/+0x28: 확보한 입자 수와 현재 초기 입자 수다.
    std::vector<ConstructionVisualParticle> particles; // +0x2c: capacity개다. 쓰지 않은 입자는 모두 0이다.
};
// 표시 생성이 공유하는 전역이다. 실제 월드/클라이언트가 소유하며 경계 뒤 현재 값을 다시 읽는다.
struct ConstructionVisualState {
    std::uint32_t processType{17}; // 패치 005410b4 / CD 0051c9a4: 부착할 buildProcess 타입이다.
    std::uint32_t palette{}; // 패치 005453cc / CD 00541848: 한 번 만든 공유 색 표다.
    float scaleX{16.0F},scaleY{11.0F}; // 패치 0059a924/28 / CD 00565c3c/40: 월드→화면 좌표 배율이다.
};
// 아직 연결하지 않은 RenderProcess·SHP 캡처·팔레트 생성과 확보 경계다. 모두 필수다.
struct ConstructionVisualHooks {
    std::function<bool(std::uint32_t)> reserve; // 원본 new의 성공 여부다. 크기 0x88·8·픽셀 수·입자 수*12를 받는다.
    std::function<void(ConstructionVisualFields&,std::uint32_t)> attach; // RenderProcess 생성/등록/부모 부착 경계다.
    std::function<std::uint32_t()> createPalette; // 8바이트 색 표 생성자가 돌려준 호스트 항목이다.
    std::function<void(std::uint32_t,std::array<double,3>,std::array<double,3>,double)> gradient; // 색수·시작/끝 RGB·2.4의 두 색 띠 생성 경계다.
    std::function<void(std::uint32_t)> buildPalette; // 두 띠를 공유 색 표로 합치는 경계다(세 번째 띠는 null).
    std::function<ConstructionVisualImage(Sid)> capture; // 현재 부모의 SHP 픽셀과 크기/hotspot이다.
    std::function<void()> releaseParticles; // Seed가 기존 입자 저장소를 새로 확보하기 전에 반납하는 경계다.
};
// 원본 생성/초기화 몸체다. RunFrame·팔레트 색 갱신·실제 화면 출력은 별도의 후속 몸체다.
class RawConstructionVisual {
public:
    // 풀·전역·난수는 이 객체보다 오래 살아야 한다. 필수 경계가 비면 첫 효과 전에 거부한다.
    RawConstructionVisual(SidPool& pool,ConstructionVisualState& state,GameRandom& random,ConstructionVisualHooks hooks);
    // 00423840/00423900 ↔ CD 004eab60/004eac30: 생성→공유 색 표→수명→캡처→동기화/입자 선택.
    // 원본의 new 실패 후 null 객체 접근은 옮기지 않고 std::bad_alloc으로 거부한다.
    ConstructionVisualFields Create(Sid parent,bool instant) const;
    // 00423770 ↔ CD 004ea630: 현재 부모 zOrder/좌표로 표시 사각형의 두 모서리를 맞춘 뒤 입자를 다시 선택한다.
    void Synchronize(ConstructionVisualFields& fields) const;
    // 00423560 ↔ CD 004ea4a0/004ea700: 불투명 수·최소 500칸 확보·마지막 픽셀 복사/제거·중앙 교차 열/
    // 하단에서 height/3까지 첫 불투명 픽셀 선택·width회 두 난수로 섞기. selected는 초기화하지 않는다.
    void Seed(ConstructionVisualFields& fields) const;
    // 어댑터가 같은 월드를 사용하는지 확인할 때 쓴다.
    const SidPool& Pool() const;
private:
    // 원본 크기 곱이 안전한 비어 있지 않은 호스트 버퍼인지 검사한다.
    std::uint32_t PixelCount(const ConstructionVisualFields& fields) const;
    // 확보 실패는 원본 null 접근 대신 예외로 거부한다.
    void Reserve(std::uint32_t bytes) const;
    SidPool& pool_; // 현재 부모 raw 자료다.
    ConstructionVisualState& state_; // 공유 색 표/현재 타입/좌표 배율이다.
    GameRandom& random_; // 다른 게임 기능과 공유하는 실제 난수 상태다.
    ConstructionVisualHooks hooks_; // 부착·캡처·팔레트·확보 경계다.
};
// 건설 시작의 표시 효과 경계를 Create에 연결하고 초기 상태의 소유권을 store에 넘긴다.
// 사용: visual과 store가 참조하는 모듈은 반환한 hooks보다 오래 살아야 한다. 실제 프레임 실행/표시는 store의 후속 책임이다.
ConstructionProcessHooks MakeConstructionVisualHooks(const SidPool& pool,const RawConstructionVisual& visual,
    std::function<void(ConstructionVisualFields)> store,ConstructionProcessHooks hooks);
}
