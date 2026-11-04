// 공통 Pop의 현재 TYPE·부분 공간 변경과 실제 하위 호출 순서를 보존한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include "o/SquidHash.h"
#include <array>
#include <functional>
#include <string_view>

namespace netstorm::o { // 같은 풀/해시/점유 지도에 원문004b02d0을 적용한다.
enum class RawPopResult; // 공개 결과 값은 기존 SquidPop.h의 정의를 공유한다.
struct SquidPopDiagnostics { // World/Session이 실제 Client의 현재 조회와 진단을 빌린다.
    RawCheckingFlag checking; // 005e4794의 현재 DWORD이며 grid 교체 진단 시점에 읽는다.
    std::span<const RiftTypeRecord> types; // 현재 raw TYPE와 초기 저장 TYPE 참조를 구별하는 전체 표다.
    std::function<std::array<float,2>(Sid)> frameSize; // 선택한 해시 단계 계산 시점에만 SHP 크기를 조회한다.
    std::function<bool()> metamap; // 00594fc8의 현재 값이며 잘못된 좌표의 로그만 생략한다.
    std::function<void(std::int32_t,std::string_view)> trace; // 원문 수준29와 완성된 로그 바이트를 같은 writer에 전달한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 진단 정상 복귀 뒤 현재 상태로 계속한다.
};
struct SquidPopParentHooks { // 도달한 실제 경계를 요구하며 하위 결과를 성공 상수로 만들지 않는다.
    std::function<const RiftTypeRecord&(std::uint8_t)> type; // 초기 TYPE 참조와 반복문의 현재 TYPE를 조회한다.
    std::function<std::array<float,2>(Sid)> frameSize; // spot 처리 뒤 실제00419850을 실행한다.
    std::function<bool()> metamap; // 좌표 보정 로그 앞에서 현재 모드를 읽는다.
    std::function<void(std::int32_t,std::string_view)> trace; // 좌표 오류와 기존 grid 교체의 원문 로그다.
    std::function<void(Sid,std::uint32_t)> miniMap; // 좌표 저장 뒤 저장 genus로 미니맵 변경을 알린다.
    std::function<void(int,int,float,float)> side; // 현재 건물 좌표와 같은 표면의 옆면 조건/쓰기/통지를 실행한다.
    std::function<void(Sid,std::uint32_t)> update; // 실제 현재 가상88/8c이며 사전 전수 검사로 옮기지 않는다.
    std::function<std::uint32_t(Sid)> firstPop; // 표시 뒤 현재 가상30을 한 번 호출한다.
    std::function<void(Sid,std::uint32_t)> activate; // firstPop을 합친 원문 flags로004ac560을 호출한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 식/파일/줄을 보존한다.
};
// 부분 좌표/spot/해시 변경을 되돌리지 않는다. CD판은 기존 별도 몸체를 유지한다.
RawPopResult RunSquidPopParent(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,Sid sid,
    float x,float y,std::uint32_t flags,RawCheckingFlag checking,const SquidPopParentHooks& hooks);
}
