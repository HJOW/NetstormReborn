// 원본사제AI의의식/건설/탐색/회수/수집·귀환전체행동을복원한다.
#pragma once
#include "o/RawAiBuildSearch.h"
#include "o/RawPathAnimation.h"

namespace netstorm::o {
//0041d7a0:각축에원본float0.99999를더한뒤signedftol하위DWORD→float로저장한다.
RawPathPoint SnapAiPriestPoint(RawPathPoint point);
//0040e800:두float모두0보다크고현재mapLimit의float저장값보다작아야한다.NaN은false다.
bool AiPriestPointValid(RawPathPoint point,std::int32_t mapLimit);
//00491b30:현재물리frame의signed첫문자-'A'가10..14인지조회한다.
bool AiPriestInRitual(const SidPool& pool,std::span<const RiftTypeFrames> frames,Sid priest);
//00448a90:다이스위치의실제제단argument WORD를적ownerDWORD와비교한다.찾은SID0도읽는다.
bool AiAltarContainsEnemy(const SidPool& pool,Sid dais,std::uint32_t enemy,std::uint32_t altarType,const std::function<Sid(float,float,std::uint32_t)>& find);
//0040f660호출직전에읽은원본여덟인자의값이다.하위가현재본체를다시읽어대신하지않는다.
struct AiPriestSearchRequest {
    std::uint32_t type{};AiBuildSearchBounds bounds;RawPathPoint avoid;std::uint32_t owner{}; //건설TYPE/자기경계/적기준점/소유자DWORD의snapshot이다.
};
// 같은원본전역의현재값을빌린다.
struct AiPriestActionGlobals {
    const double& now; //0055b4d0의현재게임시각이다.각기한쓰기위치에서재조회한다.
    const std::int32_t& mapLimit; //00531928의현재signed지도크기다.
    const std::uint32_t& daisType; //005412ec의현재다이스TYPE다.
};
// 이미복원된실제하위만선택된원문위치에서요청한다.
struct AiPriestActionHooks {
    std::function<void(std::uint32_t,std::uint32_t)> scan; //0040f700의같은574상태탐색이며prefix는0/회수뒤는1이다.
    std::function<bool(Sid)> vulnerable; //00411ca0의현재HP/최대HP판정이다.
    std::function<bool(Sid)> busy; //00425ed0→004afb40(actor,493h)예약존재다.
    std::function<Sid(Sid)> path; //00411630의현재실제경로다.
    std::function<std::int32_t(Sid)> maxHitPoints; //004adcb0의실제getter다.결과가버려져도실행한다.
    std::function<bool(Sid)> ritual; //00491b30의현재물리frame의식구간이다.
    std::function<bool(std::uint32_t,Sid)> altarEnemy; //00448a90의현재적/다이스제단argument판정이다.
    std::function<void(std::uint32_t,Sid,Sid,std::uint32_t)> target; //004116d0의실제대상이동/희생요청이다.
    std::function<double(RawPathPoint,Sid)> distance; //0041d840의같은현재발자국signed거리다.
    std::function<std::uint32_t(std::uint32_t)> select; //00411b40의실제다음건설TYPE다.
    std::function<void(std::uint32_t,Sid)> home; //004118a0의실제신전복귀다.
    std::function<void(std::uint32_t,Sid)> collect; //00411900의실제수집/귀환판단이다.
    std::function<void(std::uint32_t,std::uint32_t,RawPathPoint)> deploy; //004108f0의현재사제raw좌표/나머지인자0실제배치·로그·소리다.
    std::function<void(std::uint32_t,const AiPriestSearchRequest&)> begin; //0040f660의같은실제탐색초기화다.
    std::function<void(std::uint32_t,Sid,RawPathPoint)> point; //004117e0의실제좌표이동/희생0이다.
    std::function<bool(std::uint32_t)> enemyLowFunds; //00411ad0의현재실제적자금조건이다.
    std::function<void(Sid)> salvage; //0044c420의실제회수요청이며직접환불/삭제로우회하지않는다.
};
// 원본00411cd0전체를같은0xb08/실제pool과같은행동하위에서수행한다.자동프레임순서는별도다.
class RawAiPriestActions {
public:
    RawAiPriestActions(RawAiBrainMemory& memory,const SidPool& pool,AiPriestActionGlobals globals,AiPriestActionHooks hooks); //같은PATCH풀/실제본체를빌린다.
    void Act(std::uint32_t token,Sid priest) const; //탐색prefix→사제관문→의식→건설/후보이동/회수/수집·귀환이다.
    const SidPool& Pool() const; //같은실제월드연결을확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재살아있는실제0xb08본체만허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //현재DWORD를읽는다.
    void Field(std::uint32_t token,std::size_t at,std::uint32_t value) const; //지정DWORD만쓴다.
    double Time(std::uint32_t token,std::size_t at) const; //현재QWORD를float축소없이읽는다.
    void Deadline(std::uint32_t token,std::size_t at,double delay) const; //현재now+원본double기한을쓴다.
    bool Ready(std::uint32_t token,std::size_t at) const; //now>=due이며unordered는미도달이다.
    Sid Object(std::uint32_t value) const; //DWORD를물리검사뒤WORD SID로바꾼다.0은물리슬롯이다.
    RawPathPoint Position(Sid actor) const; //현재pool의raw두float다.
    RawPathPoint Target(std::uint32_t token) const; //현재ad0/ad4목표다.
    void Target(std::uint32_t token,RawPathPoint point) const; //목표두float만쓴다.
    void ClearTarget(std::uint32_t token) const; //원본순서대로y→x를-10000비트로쓴다.
    void BeginSearch(std::uint32_t token,std::uint32_t type) const; //현재8인자를snapshot뒤실제초기화에넘긴다.
    void Fallback(std::uint32_t token,Sid priest) const; //2cc/현재HP선저장/최대HP/능력800h별수집또는귀환이다.
    Sid BestBuilding(std::uint32_t token) const; //300/308최초목록에서signed HP최대의첫SID를고른다.free/dead/owner필터는없다.
    RawAiBrainMemory& memory_; //다른AI와같은본체/목록저장소다.
    const SidPool& pool_; //현재raw위치/HP/목록대상원문이다.
    AiPriestActionGlobals globals_; //현재시각/지도/다이스전역의참조다.
    AiPriestActionHooks hooks_; //선택된실제하위효과다.
};
}
