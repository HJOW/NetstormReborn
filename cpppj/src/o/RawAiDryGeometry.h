// 가짜 부족좌표의 원본 선분/수직 양방향 탐색과 실제 디버그 표식 수명을 복원한다.
#pragma once
#include "o/RawAiBuildSearch.h"

namespace netstorm::o {
//0041cfc0:두 float 좌표 차이의 제곱합/제곱근을 중간 float 저장 없이 반환한다.
double AiPointDistance(RawPathPoint first,RawPathPoint second);
// 원본 현재 경계/디버그 종류/표식 TYPE의 같은 저장소를 빌린다.
struct AiDryGeometryGlobals {
    const AiBuildSearchBounds& bounds; //00544d14..20의 현재 네 float이며 양끝을 포함한다.
    const std::uint32_t& debug; //0052f3dc가29일 때만 실제 표식을 만든다.
    const std::uint32_t& markerType; //005411f8의 현재 TYPE이며 초기값104다.
};
// 이미 복원된 실제 지도/가상 생성·kind WORD·Pop/삭제만 선택 위치에서 호출한다.
struct AiDryGeometryHooks {
    std::function<std::uint16_t(std::int32_t,std::int32_t)> surface; //현재00542514의256폭 WORD 지도다.범위 밖은 호출하지 않는다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //004af530의 현재TYPE/flags2다.
    std::function<void(Sid,std::uint16_t)> word; //004ac220의kind93 쓰기와 현재 다리 통지다.
    std::function<void(Sid,RawPathPoint,std::uint32_t)> pop; //실제 가상+90의좌표/flags0이다.
    std::function<void(Sid,std::uint32_t)> destroy; //실제 가상+10의flags0 삭제다.
};
// 별도 지도나 후보 사본을 만들지 않고 같은 실제 풀/경계/현재 효과를 사용한다.
class RawAiDryGeometry {
public:
    RawAiDryGeometry(const SidPool& pool,AiDryGeometryGlobals globals,AiDryGeometryHooks hooks); //PATCH 풀과 현재 전역/실제 효과를 빌린다.
    RawPathPoint Candidate(RawPathPoint source,RawPathPoint target) const; //00415bb0:선분 방문→중점 수직 양방향→연속 빈칸4개다.원본의 반복 예산 부재도 유지한다.
    void ClearMarkers() const; //00414d30:SID1을 건너뛴 live iterator/다음 먼저 선택/객체TYPE BYTE와현재TYPE 전체DWORD 및kind93 일치만 가상 삭제한다.
    void Mark(RawPathPoint point) const; //00410540:현재TYPE/flags2 생성→kind93→실제Pop(flags0)이다.
    const SidPool& Pool() const; //같은 실제 월드 연결을 확인한다.
private:
    bool Inside(RawPathPoint point) const; //매 위치의 현재 네 경계를 양끝 포함/NaN 제외로 검사한다.
    bool Empty(RawPathPoint point) const; //이동 뒤 각 축+원본0.9999 편향을ftol하고256×256 지도만 읽는다.
    const SidPool& pool_; //표식 순회와 실제 가상 효과가 공유하는 PATCH 풀이다.
    AiDryGeometryGlobals globals_; //경계/디버그/TYPE의 현재 참조다.
    AiDryGeometryHooks hooks_; //현재 지도와 선택된 실제 표식 수명 효과다.
};
}
