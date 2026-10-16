// Lightning.cpp의 공유 빔 경로와 재귀 입자 생성을 원본 저장 폭으로 복원한다.
#pragma once
#include "o/SidPool.h"
#include "o/SpStore.h"
#include <functional>

namespace netstorm::o {
// 원본24BYTE 빔 입자다. 이전 입자 포인터는 같은 고정 저장소의 index+1 토큰으로 표현한다.
struct LightningParticle {
    float x{},y{}; // 생성 시 snapshot인 픽셀 좌표다. 이후 표시 프레임의 jitter와 구별한다.
    float strength{},jitterWeight{},life{}; // 가지의 굵기·흔들림 가중치·10배 시간 수명이다.
    std::uint32_t previous{}; // 0은 NULL이며 나머지는 같은 저장소의 이전 입자 index+1이다.
};
// 실제 용량 전체와 Append 꼬리를 함께 소유한다. 경로 생성은 죽은 입자를 재사용하지 않는다.
struct LightningParticles {
    std::vector<LightningParticle> values; // 생성자가 확보할 실제 용량 전체다. 정식 빔은120개를0으로 초기화한다.
    std::int32_t total{}; // 사용한 꼬리 수이며 완료 뒤에도 줄이지 않는다.
    std::uint32_t owner{}; // 경로와 같은 소유 빔 토큰이다. 다른 빔의 입자 혼용을 거부한다.
};
// 원본64BYTE 경로 이미지다. 생성자가 쓰지 않는+24..+34도 원문처럼 보존한다.
struct LightningPathFields {
    std::array<std::uint8_t,64> bytes{}; // float/DWORD 전체 저장 비트이며+34/+3c만 호스트 토큰이다.
};
// 원본 경로 생성 시점에 읽는 실제 viewport 배율과 타입 전역이다.
struct LightningPathState {
    float scaleX{16},scaleY{11}; // 같은 게임의 픽셀/타일 배율이다.
    std::uint32_t vanderType{148},tallType{149},manOWarType{144}; // 원본 source/target 높이 보정의 현재 타입이다.
};
// 같은 raw 풀·게임 난수와 현재 중심 조회를 사용하며 표적 SID를 경로에 캐시하지 않는다.
class RawLightningPath {
public:
    // 모든 참조/중심 소비자는 이 모듈보다 오래 살아야 한다.10.78만 복원한 상태다.
    RawLightningPath(const SidPool& pool,GameRandom& random,const LightningPathState& state,
        std::function<std::array<float,2>(Sid)> center);
    // 00471b20: 생성 시 양끝/거리/각도를 snapshot하고 미사용 바이트는 보존한다.
    void Initialize(LightningPathFields& fields,Sid source,Sid target,std::uint32_t owner) const;
    // 004720f0: 흔들리지 않는 시작점을 추가하고 step0.15·진행0·기본 각도를 저장한다.
    void Seed(LightningPathFields& fields,LightningParticles& particles) const;
    // 00472180: 현재 진행까지 재귀 가지를 만들며1에 도달하면 끝점을 한 번 추가한다.
    void Step(LightningPathFields& fields,LightningParticles& particles,float progress,float lifeOffset) const;
private:
    // 손상된 꼬리/다른 소유 빔은 상태를 바꾸기 전에 거부한다.
    static void Validate(const LightningPathFields& fields,const LightningParticles& particles);
    // 00487b20: 빈 꼬리만 증가시킨다. 원본 용량 초과 assert는 명시 예외로 보고한다.
    static std::uint32_t Append(LightningParticles& particles);
    // 현재 입자 토큰을 같은 저장소의 원래 항목으로 해석한다.
    static LightningParticle& At(LightningParticles& particles,std::uint32_t token);
    // 실제00455930의 상위16비트/65536·절댓값 간격·넓은 반환을 유지한다.
    double Random(float low,float high) const;
    // 00471fc0: cos/sin 점을 추가한 뒤 조건에 따라 길이/굵기를 반으로 줄여 재귀한다.
    std::uint32_t Branch(LightningPathFields& fields,LightningParticles& particles,
        float angle,float length,std::uint32_t previous,float strength,float lifeOffset) const;
    const SidPool& pool_; // 생성 시 원래 source/target 타입을 읽는 같은 저장소다.
    GameRandom& random_; // 다른 표시/게임 계산과 공유하는 실제 난수 상태다.
    const LightningPathState& state_; // 생성 시마다 현재 배율/타입을 읽는다.
    std::function<std::array<float,2>(Sid)> center_; // 같은 월드의 실제004adea0 중심 조회다.
};
}
