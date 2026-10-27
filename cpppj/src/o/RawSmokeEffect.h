// Smoke.cpp의 냉각 연기 생성·공통 입자/시간·프레임·색/선 요청 수명을 복원한다.
#pragma once
#include "o/RawFieryTailFrame.h"

namespace netstorm::o { // 같은 원본 공용 입자/표시 계층의 처리기다.
// 원본128BYTE의 의미 필드이며 실제 포인터는 소유 vector/등록 토큰으로 표현한다.
struct SmokeFields {
    RenderProcessFields registration; // 공통 form/부모/영역/목록 슬롯/깊이다.
    std::vector<FieryTailParticle> particles; // 동일24BYTE 입자 전체 용량이다.
    std::int32_t capacity{},total{}; // +24/+28이며 소멸 후에도 원본 숫자 필드는 유지한다.
    bool hasStorage{}; // +2c의 유효 저장소 존재다. 소멸 뒤 해제 주소를 역참조하지 않는다.
    double lastTime{},elapsed{}; // +30/+38의 실제 타이머다.
    float delta{},scaledDelta{}; // +40/+44이며 생성자가 쓰지 않는 입력을 보존한다.
    bool first{}; // +48의 최초 시각/영역 초기화 여부다.
    float due{},windX{},windY{}; // +50/+54/+58의 다음 방출 시각과 개별 바람이다.
    std::int32_t hotspotX{},hotspotY{},remaining{}; // +5c/+60/+64의 포구 픽셀 오프셋과 남은 방출 수다.
    float maximumLife{100},velocity{-1},spread{9},period{5},noise{.5F}; // +68..+78의 수명/속도/위치 분산/간격/속도 분산이다.
};
// 실제004a8140의 부모와10개 인자 중 설정 가능한 생성 값이다.
struct SmokeRequest {
    Sid parent{}; // 같은 raw 부모 SID다.
    std::int32_t hotspotX{},hotspotY{}; // 현재 SHP +20/+22에서 읽은 signed WORD의 확장값이다.
    float windX{},windY{},maximumLife{100},velocity{-1},spread{9},period{5},noise{.5F}; // 실제 float 생성 인자를 순서대로 보존한다.
};
// 모든 연기 효과가 같은 월드에서 재사용하는 원본005c7aa8의 공유 색 객체다.
struct SmokeLifetimeState { std::uint32_t palette{}; /* palette: 첫 생성 뒤 재사용하는 비영 토큰이다. */ };
// 실제 소멸의 입자 버퍼와 scalar deleting destructor 객체 반납을 구별한다.
enum class SmokeRelease { Particles,Object };
// 실제 vector/공유 색 확보만 외부 소유자에 연결한다.
struct SmokeLifetimeHooks {
    std::function<bool(std::uint32_t)> reserve; //200×24 입자/8BYTE 색 객체의 실제 요청이다.
    std::function<std::uint32_t()> createPalette; // 실제 공유 색 객체 확보가 끝난 비영 토큰이다.
    std::function<void(std::uint32_t)> buildPalette; // 원문77..88의12 DWORD를 실제48BYTE 색 배열에 만든다.
};
// 실제 Frame/Draw가 소비하는 장치와 같은 Host의 자기 삭제다.
struct SmokeFrameHooks {
    std::function<void(FieryTailDirtyRect)> invalidate; // 이전/새 영역의 원문 dirty 통지다.
    std::function<void(FieryTailLine)> line; // 같은 y에서 x..x+1인 실제2픽셀 선 요청이다.
    std::function<void(std::uint32_t)> kill; // 살아 있던 입자가 없으면 같은 프로세스를 flags0으로 없앤다.
};
// 원본00487cd0(77,88)의 요청 수/팔레트 번호 순서를 반환한다. RGB 근사 색을 만들지 않는다.
std::array<std::uint32_t,12> BuildSmokePalette();
// 실제 공통 registry 생성/200입자·공유 색 초기화와 반납을 소유한다.
class RawSmokeLifetime {
public:
    // registry/state/hooks는 처리기보다 오래 살아야 한다.
    RawSmokeLifetime(const SidPool& pool,RawRenderRegistry& registry,SmokeLifetimeState& state,SmokeLifetimeHooks hooks);
    //004a8140: 공통 등록/부착 뒤 입력 값·200입자·공유 표를 초기화한다.
    void Initialize(SmokeFields& fields,std::uint32_t token,const SmokeRequest& request) const;
    //004a86e0: 같은 기본 생성자를 끝낸 뒤 count10/period3을 덮어쓴다.
    void InitializeCooling(SmokeFields& fields,std::uint32_t token,Sid parent,std::array<std::int32_t,2> hotspot) const;
    // 입자→공통 등록/base→flags1 객체 순서로 반납하며 공유 색은 남긴다.
    void Destroy(SmokeFields& fields,std::uint32_t flags,const std::function<void(SmokeRelease)>& release) const;
private:
    RawRenderRegistry& registry_; // 공통 표시 목록이다.
    SmokeLifetimeState& state_; // 같은 월드의 공유 색 객체다.
    SmokeLifetimeHooks hooks_; // 실제 입자/공유 색 확보 경계다.
};
//004a8240/004a82b0/004a85a0의 실제 단일 프레임/방출/출력을 처리한다.
class RawSmokeFrame {
public:
    // 현재 시각/카메라와 읽을 수 있는 색 메모리를 같은 표시 세계에서 받는다.
    RawSmokeFrame(const SidPool& pool,GameRandom& random,const FieryTailFrameState& state,FieryTailFramePalette& palette,SmokeFrameHooks hooks);
    // 빈 슬롯을 확보한 경우에만 속도 난수 두 개와 좌표/수명을 쓴다.
    void Emit(SmokeFields& fields,float x,float y,float life) const;
    // 최초 시각/영역 또는 전체 경과 시간 한 번의 생성/분열·이동·dirty/종료를 처리한다.
    void Step(SmokeFields& fields) const;
    // 좌표를 먼저 절삭한 뒤 카메라를 빼고 원문 색의2픽셀 선을 제출한다.
    void Draw(const SmokeFields& fields) const;
private:
    // 현재 꼬리 또는 앞에서 찾은 life<=0 슬롯을 사용하며 NaN은 재사용하지 않는다.
    FieryTailParticle* Append(SmokeFields& fields) const;
    // 같은 GameRandom의 실제 float 구간 난수이며 반환은 넓은 중간값이다.
    double Random(float low,float high) const;
    // 현재 입자 수/용량과 실제 저장소를 검증한다.
    void Validate(const SmokeFields& fields) const;
    // 원본00487ba0의 이전/새 dirty를 같은 현재 카메라로 제출한다.
    void Invalidate(float left,float top,float right,float bottom) const;
    // 원본 색수/float 역수의 조회다. 인접 DWORD는 공급된 실제 메모리만 읽는다.
    std::uint32_t Color(float life,float maximum) const;
    const SidPool& pool_; // 현재 부모 raw 좌표 저장소다.
    GameRandom& random_; // 다른 게임 로직과 공유하는 난수다.
    const FieryTailFrameState& state_; // 현재 시각/픽셀 배율/카메라다.
    FieryTailFramePalette& palette_; // 공유12색과 실제 읽을 수 있는 인접 메모리다.
    SmokeFrameHooks hooks_; // 같은 dirty/선 출력과 실제 자기 삭제 경계다.
};
}
