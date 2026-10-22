// 원본 AI 전체행동·개별프레임·Player1..8 자동방문/삭제 수명을 복원한다.
#pragma once
#include "o/RawAiBrain.h"

namespace netstorm::o {
// 각 원본 조회 위치에서 현재 전역을 읽는다.
struct AiRuntimeGlobals {
    std::function<bool()> battle; //00594fbc의현재전투모드이며server/boss와합치지않는다.
    const std::uint32_t& timing; //0052f3c4의현재실행시간로그활성DWORD다.
    const std::uint32_t& dryLog; //0052f3dc의현재부족좌표로그종류다.
};
// 현재 동일본체의 실제 행동·외부시계/로그·소유자만 연결한다.
struct AiRuntimeHooks {
    std::function<void(std::uint32_t)> initialize,prepare,selectGoal; //00414af0/00416eb0/004139f0의실제준비순서다.
    std::function<void(std::uint32_t)> collectors,factories; //00415a10→004136b0의현재행동이다.
    std::function<std::uint32_t(std::uint32_t)> dryScan; //0040f2b0의제거TYPE이며0은제거없음이다.
    std::function<void(std::uint32_t,std::uint32_t,bool)> dryNotice; //004105b0의현재fakeTYPE비교결과와현재로그종류다.
    std::function<void(std::uint32_t,std::uint32_t)> priest; //00411cd0의현재29c전체DWORD다.번호축소는실제월드경계에서검사한다.
    std::function<void(std::uint32_t)> drops,bridges,carriers,economy; //004162e0→00417bd0→00412130→004138c0의실제순서다.
    std::function<float(std::uint32_t)> money; //0040ef30의현재owner SP float다.
    std::function<double()> wall; //00460d70의매호출현재벽시각이며게임시각snapshot으로대체하지않는다.
    std::function<void(std::uint32_t,double)> took; //0.1초를엄격히넘을때현재29로그장치에전달한다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //004e0620의현재진단이며반환하면원본처럼계속한다.
    std::function<void(std::uint32_t)> destruct,free; //현재Player+b0본문의실제00410100→CRTfree다.
};
//00417c70/00417d30/00417de0의순서와현재필드재조회만수행한다.
class RawAiRuntime {
public:
    RawAiRuntime(RawAiBrainMemory& memory,std::span<std::uint8_t> players,AiRuntimeGlobals globals,AiRuntimeHooks hooks); //같은9×180BYTE Player와실제본체저장소를빌린다.
    void Actions(std::uint32_t token) const; //수집자→시설→부족좌표→사제→배치→다리→운반자→경제→float지갑검사다.
    bool Tick(std::uint32_t token) const; //254가비제로일때만false이며초기화미완료는본문을유지한다.
    void Frame() const; //현재전투에서Player1..8의현재brain을방문하고false반환본문을소멸/반납뒤0으로쓴다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //살아있는정확한0xb08만허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //현재비정렬DWORD를조회한다.
    RawAiBrainMemory& memory_; //기존생성자/행동과같은실제소유자다.
    std::span<std::uint8_t> players_; //현재9Player의유일한저장소다.
    AiRuntimeGlobals globals_; //현재전역의참조/getter다.
    AiRuntimeHooks hooks_; //실제행동/장치/수명의선택된하위다.
};
}
