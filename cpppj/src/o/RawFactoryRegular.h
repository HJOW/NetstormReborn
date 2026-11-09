// Factory4549c0의 현재 레벨/프레임/난수와 선택된 큰 업그레이드 효과를 공유한다.
#pragma once
#include "o/SquidProcess.h"
namespace netstorm::o { // 두 직접 VT와 원문의 비가상 기본 호출이 같은 본문을 사용한다.
struct FactoryRegularHooks { // 조회와 실제 가상 효과만 경계로 두고 사건/호출 순서는 본문이 결정한다.
    std::function<std::int32_t(Sid)> level; // 현재 가상80이며 두 조회를 합치지 않는다.
    std::function<std::int32_t(Sid,std::int32_t)> firstFrame,frameCount; // 레벨 조회 뒤의 현재 TYPE+130/+170이다.
    std::function<std::uint32_t(std::uint32_t)> random; // 같은 현재00532710이며0범위도 원문 선행seed 쓰기를 보존한다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> frame; // 실제4acee0의 Frame.Set이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> advance; // 실제4afc90의 한 번 감싸기 프레임 진행이다.
    std::function<void(Sid,float)> begin; //count0의 실제 전체 예측 그림/건설/감시쌍 생성이다.
    std::function<float(Sid)> finish; //비0 count의 실제 완료·현재AI 통지·-1 반환이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> flare; //현재 레벨>2인 사건5의 실제2/1 플레어다.
    RegularHandler damageable; //선택된 조기 반환 이외의 실제44b900이다.
};
float RunFactoryRegular(const SidPool& pool,Sid parent,std::uint32_t event,std::uint32_t count,float payload,const FactoryRegularHooks& hooks); //현재 조회 순서와 DWORD 프레임 덧셈 감김을 보존한다.
}
