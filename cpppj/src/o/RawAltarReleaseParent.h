// 패치 제단 해방의 현재 진단과 검색 결과/가상 공간/표시 없는 프레임 쓰기 순서를 보존한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 실제 하위 몸체/장치만 빌리며 포획/희생 시뮬레이션을 추가하지 않는다.
// 00448bd0의 직접 호출 경계이며 선택한 경계만 요구한다.
struct AltarReleaseParentHooks {
    std::function<Sid(Sid)> findDais; // 진단 전에 현재 위치/다이스 TYPE으로 최초 검색한 SID다.
    std::function<std::int32_t(Sid)> direction; // 첫 결과12가 아니면 두 번째 현재 방향을 읽는다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 정상 복귀하면 저장한 검색 결과로 계속한다.
    std::function<bool(Sid)> valid; // 진단 뒤 현재 실제 풀의 포인터 범위를 검사한다.
    std::function<void(Sid)> release; // 유효한 저장 다이스의 기존00448500 전체 몸체다.
    std::function<void(Sid,std::uint32_t)> unpop,repop; // 현재 제단의 가상+48/+4c이며 flags0이다.
    std::function<void(Sid)> plainP; // Unpop 뒤 현재 TYPE의 P 첫번호를 표시 통지 없이 raw 프레임에 쓴다.
};
// 원문은 첫 finder 결과를 저장하고 진단/해방 뒤 현재 제단 공간/TYPE을 조회한다.
void RunAltarReleaseParent(Sid altar,RawCheckingFlag checking,const AltarReleaseParentHooks& hooks);
}
