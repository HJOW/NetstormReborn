// 실제 기술 내용물과 직전 후보/표적·방향·쿨다운으로 AI의 다음 배치 TYPE를 선택한다.
#pragma once
#include "o/RawAiDropScan.h"

namespace netstorm::o {
// 원본 세 로그의 의미이며 출력 여부/현재 로그 종류는 원래 위치에서 판단한다.
enum class AiDropSelectionLog { Hated,Retried,Scanning }; //직전표적선호/후보재사용/현재TYPE·회전알림이다.
// 이미 복원된 같은 기술 목록/전체 배치·후보 압축과 실제 로그를 연결한다.
struct AiDropSelectionHooks {
    std::function<Sid(std::uint32_t)> technology; //0048fc90의현재owner기술부모다.선택전체에서는최초부모를보존한다.
    std::function<std::int32_t(Sid)> count; //004acc60(mask8,kind0)의실제모든일치개수다.중복도센다.
    std::function<Sid(Sid)> blocker; //004acb80(mask8,genus4000000,index0)의현재실제블로커생산조각이다.
    std::function<std::uint32_t(Sid,std::uint32_t)> nth; //004acac0(mask8,kind0,index)의TYPE WORD다.SID가아니다.
    std::function<bool(const CanonPlacementQuery&)> mayPlace; //0049b510전체:argument0/flags0/현재ownerBYTE/mode1이다.
    std::function<void(std::uint32_t,std::int32_t)> remove; //0040f390의같은30후보안정압축이다.
    std::function<void(std::uint32_t,AiDropSelectionLog,std::uint32_t,std::int32_t)> log; //004105b0의현재출력이다.b00비영인실제위치에서만요구한다.
};
// 실제 본체/기술/공용 난수와 현재 네 방향 TYPE 전역만 사용한다.
class RawAiDropSelection {
public:
    RawAiDropSelection(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint32_t> directionalTypes,GameRandom& random,const double& now,AiDropSelectionHooks hooks); //현재네TYPE는00541264/25c/288/290의순서이며기본131/129/140/142다.
    void Select(std::uint32_t token) const; //00413100의진행초기화→기술개수→반복/블로커우선→회전·순환→현재후보/기한정리다.
    bool Directional(std::uint32_t type) const; //00425830의현재네DWORD와전체TYPE번호를순서대로비교한다.
    const SidPool& Pool() const; //같은실제월드연결확인용이다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재살아있는실제0xb08본체만허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //하위효과뒤현재DWORD를읽는다.
    void Field(std::uint32_t token,std::size_t at,std::uint32_t value) const; //지정DWORD만쓰고주변을보존한다.
    const RiftTypeRecord& Type(std::uint32_t type) const; //선택된원문위치에서현재TYPE0도조회한다.
    bool Contains(std::uint32_t token,std::uint32_t target) const; //0040e9d0(this+36c)의최초목록포인터/현재count/첫일치다.
    void Log(std::uint32_t token,AiDropSelectionLog action,std::uint32_t type=0,std::int32_t direction=0) const; //각원문위치의현재b00출력조건이다.
    double Time(std::uint32_t token,std::size_t at) const; //현재QWORD를float축소없이읽는다.
    RawAiBrainMemory& memory_; //같은본체/기술순번·후보/직전표적저장소다.
    const SidPool& pool_; //같은실제월드정체성이다.
    std::span<const RiftTypeRecord> types_; //현재flags2와진단TYPE를읽는다.
    std::span<const std::uint32_t> directionalTypes_; //네현재원본방향TYPE전역의참조다.
    GameRandom& random_; //SP/시설/스캔과같은공용난수다.
    const double& now_; //0055b4d0의현재QWORD시각이다.
    AiDropSelectionHooks hooks_; //선택된실제하위/로그다.
};
}
