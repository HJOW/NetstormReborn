// 공통 Halt의 현재 진단/저장 부모/자기 삭제 뒤 복귀와 실제 후속 경계를 보존한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 원본 stack deadFlag의 주소 대신 같은 경로의 수명 관찰을 빌린다.
// 00489ff0의 선택된 실제 조회/가상 호출이며 삭제 후 alive/checking/진단만 사용한다.
struct PathHaltParentHooks {
    std::function<Sid()> parent; // 가상 Halt 전에 저장할 최초 부모다.
    std::function<std::uint32_t(Sid)> halt; // 같은 부모+38의 원문 결과10/11/12다.
    std::function<bool()> alive; // 경로 소멸 뒤에도 유효한 별도 수명 관찰이다.
    std::function<bool(Sid)> forcedStop; // 결과10 뒤 저장 부모의 현재 flags40 비트20이다.
    std::function<bool()> mayKill; // 현재 boss 또는 현재 Form의 로컬 범위를 판정한다.
    std::function<void()> kill; // 같은 Host가 경로를 flags0으로 제거한다.
    std::function<bool()> boss; // 재지정 앞에서 다시 읽는 현재 권한이다.
    std::function<bool(Sid)> carrier; // 저장 부모의 현재 TYPE genus30000이다.
    std::function<bool()> hasTargets; // 현재 TargetB 또는 TargetA이며 조회 순서를 유지한다.
    std::function<void(Sid)> retarget; // 기존 내용물/후보/새 이동 몸체이며 자신을 삭제할 수 있다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 삭제 후에도 장치 수명이 유지되며 정상 복귀하면true다.
    std::function<void()> armWatch,clearWatch; // raw 이미지의 비주소 관찰 토큰 경계이며 독립 수명 관찰만 쓰는 caller는 생략한다.
};
// checking은 caller 소멸 뒤에도 유효한 현재 DWORD 뷰를 값으로 빌린다.
bool RunPathHaltParent(RawCheckingFlag checking,const PathHaltParentHooks& hooks);
}
