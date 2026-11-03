// 공통 도착의 현재 진단/공간·종료와 삭제 뒤 원문 결과 분기를 보존한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/RawPathRuntime.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 물리 주소/지역 watch는 호스트 수명 관찰과 구별한다.
class SquidHash; // 실제 일반 finder가 같은 월드의 네 단계 버킷을 빌린다.
// 원문 spot8/일반 finder/현재 genus mask의 도착 겹침 판정이며 기존 경로 몸체를 공유한다.
bool PathArrivalBlocked(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> spots,const std::uint32_t& blockingGenus,RawPathPoint point);
// 0041be80은 SID 범위를 진단한 뒤 항상1을 반환한다. 현재 조회와 진단만 공급한다.
struct PathBaseRobustHooks {
    std::function<std::uint32_t()> form,parent,capacity; // 현재 base+4/+8과 실제 풀 상한이다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 정상 복귀 뒤 현재 부모 조회를 계속한다.
};
// 유효 물리 SID 범위의 의미를 보존하며 Host 등록/생존 필터를 추가하지 않는다.
bool RunPathBaseRobust(const PathBaseRobustHooks& hooks);
// 0048b340이 선택하는 실제 하위 몸체이며 객체 삭제 뒤에도 alive/진단/clear는 유효해야 한다.
struct PathArrivalParentHooks {
    std::function<Sid()> parent; // 최초 저장할 현재 경로 부모다.
    std::function<bool(Sid)> validAlive; // 최초 부모의 범위와dead/free 비트만 검사한다.
    std::function<RawPathPoint()> step; // 기존 실제 Step의 최종 좌표이며 현재 경로를 갱신한다.
    std::function<void(Sid)> unpop; // 저장 부모의 가상+48 flags0이다.
    std::function<void(Sid,RawPathPoint)> pop; // 저장 부모의 가상+90 flags0이다.
    std::function<bool()> alive; // 객체 밖에 보존한 같은 경로의 수명 관찰이다.
    std::function<void()> robust; // 원문 항상참 함수의 현재 SID 진단 부작용을 실행한다.
    std::function<bool(RawPathPoint)> blocked; // 현재 spot8/일반 finder의 실제 부류 판정이다.
    std::function<bool()> halt; // 같은 실제 경로의 가상+28이다.
    std::function<void(bool)> finished; // 현재 경로 finished DWORD를1/0으로 쓴다.
    std::function<bool(Sid)> deadOrFree; // Halt 뒤 저장 부모의 현재 두 상태 비트다.
    std::function<bool()> watchPresent; // 현재 원문 deathWatch 필드의 비영 여부다.
    std::function<void()> armWatch,clearWatch; // 호스트의 비주소 관찰 토큰을 설정/해제한다.
    std::function<std::uint32_t(Sid)> arrival; // 현재 두 target DWORD로 같은 부모+3c를 호출한다.
    std::function<void()> kill; // 같은 Host의 flags0 제거다.
    std::function<void(Sid)> clear; // 삭제 뒤에도 저장 부모의 현재 위치/TYPE/owner로 겹침을 정리한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 진단 정상 복귀를 보존한다.
};
// 정상 원문의 효과 순서와 선택된 현재 진단을 보존하며 물리 객체 삭제 뒤 역참조를 막는다.
void RunPathArrivalParent(RawCheckingFlag checking,const PathArrivalParentHooks& hooks);
}
