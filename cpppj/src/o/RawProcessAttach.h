// ProcessForm의 파생 +74 접두를 공통 Form 부착과 구별한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 현재 서버와 진단/선택 장치를 원문 소비 순서에 제공한다.
struct ProcessAttachHooks { // 선택되지 않은 직렬화/송신 장치는 요구하지 않는다.
    std::function<bool()> server; // 부모 변경 진단 뒤 현재 서버 전역을 읽는다.
    std::function<void(Sid)> lookup; // 서버이면 로컬 flags에서도 현재 PID의 프로세스를 먼저 찾는다.
    std::function<std::int32_t(bool)> writeData; // 저장한 프로세스의 실제 writer와 최초 부모 변경 판정을 사용한다.
    std::function<void(std::int32_t)> transmit; // 양수 길이 진단 정상 복귀 뒤에만 선택된다.
    std::function<void(Sid,Sid,std::uint32_t)> attach; // 파생 접두 뒤 현재 공통 Form 부모로 넘긴다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 진단 정상 복귀와 명시 중단을 구별한다.
};
void AttachProcessForm(SidPool& pool,Sid form,Sid parent,std::uint32_t flags,const ProcessAttachHooks& hooks); //004b1440/CD004af380의 진단·선택·공통 부착 순서다.
}
