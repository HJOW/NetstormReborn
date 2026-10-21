// 원본 AI의 원소별 기술 선택00413570과 시설 생산/대기 목록004136b0을 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawContainedFinder.h"

namespace netstorm::o {
// 원본 전략이 매 판단에서 읽는 현재 시간과 무효 그래프다.
struct AiFactoryProductionGlobals {
    const double& now; //0055b4d0의 현재 QWORD 게임 시각이다.
    std::function<std::uint32_t()> invalidGraph; //00540410의 현재 무효 그래프 번호다.
};
// 원본 행동이 기록하는 두 로그의 의미다.
enum class AiFactoryAction { AddedProduct,Upgrade }; //기술 추가 또는 업그레이드 요청 뒤 생각 기한을 갱신한다.
// 같은 Player/생산/가상 level/업그레이드와 진단 장치에 연결하는 하위다.
struct AiFactoryProductionHooks {
    std::function<Sid(std::uint32_t)> technology; //0048fc90의 현재 owner 기술부모다.
    std::function<Sid(std::uint32_t,std::uint32_t)> product; //00410360의 같은 본체→현재 기술부모 mask8 조회다.
    std::function<void(std::uint32_t,Sid,std::uint32_t)> addProduct; //00410420의 실제 로컬/원격 생산 등록이다.
    std::function<std::uint32_t(Sid)> level; //가상+80/00453f40의 현재 시설 level이다.
    std::function<void(Sid)> upgrade; //00453bf0의 실제 업그레이드 요청이다. 미복원 호출은 선택된 위치에서 거부한다.
    std::function<void(std::uint32_t,AiFactoryAction,std::uint32_t)> log; //현재 원본 로그 종류와 TYPE 이름을 실제 장치에 전달한다.
    std::function<bool()> demo; //00411180의현재 데모 지갑 하한 여부다.
};
// 현재 기술 내용물과 같은 본체의 미처리/업그레이드 큐만 사용한다.
class RawAiFactoryProduction {
public:
    RawAiFactoryProduction(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,
        const ContainedFinderState& finder,AiFactoryProductionGlobals globals,AiFactoryProductionHooks hooks); //같은 PATCH/TYPE와 실제 내용물 순회 상태를 빌린다.
    std::uint32_t SelectTechnology(std::uint32_t token,std::int32_t theme) const; //00413570의 첫 일치 미등록 기술과 battery 원소 치환을 선택한다.
    std::int32_t CountProducts(Sid factory) const; //004acc60(mask10,kind0)의 실제 내용물 총수다. 같은 종류도 각각 센다.
    void Manage(std::uint32_t token) const; //004136b0의 생산 추가/목록 이동/업그레이드 요청 경계를 실행한다.
    void Upgraded(std::uint32_t token,Sid factory) const; //00411180:같은 시설을 미처리 목록에 추가→현재TYPE의 음수 비용 절삭 가산→데모1190 하한이다.
    const SidPool& Pool() const; //현재 세션 조립의 같은 풀인지 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재0xb08 본체를 물리 검사 뒤 다시 빌린다.
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; //현재 DWORD를 읽는다.
    void Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const; //선택된 DWORD만 쓰고 나머지를 유지한다.
    std::uint32_t Item(std::uint32_t token,std::size_t pointer,std::int32_t index) const; //현재 목록 backing 안의 DWORD를 읽는다.
    void Remove(std::uint32_t token,std::size_t pointer,std::uint32_t value) const; //0040ea00의 모든 중복 안정 압축과 count 감소다.
    void Append(std::uint32_t token,std::size_t pointer,std::uint32_t value) const; //0040e9b0의 여유 있을 때 중복 허용 추가다.
    Sid PhysicalSid(std::uint32_t value) const; //DWORD 물리 범위를 먼저 확인하고 WORD로 좁힌다.
    const RiftTypeRecord& Type(std::uint32_t type) const; //현재 TYPE0도 원본처럼 읽으며 표 밖은 거부한다.
    std::int32_t Theme(Sid factory) const; //004ac970의 현재 BYTE TYPE→theme이다.
    bool Ready(std::uint32_t token) const; //현재5a8 QWORD 기한과 비교하며 unordered는 거짓이다.
    void Action(std::uint32_t token,AiFactoryAction action,std::uint32_t type) const; //00410620의 생각 기한 두 개와 조건부 로그다.
    RawAiBrainMemory& memory_; //기존 생성/전략/통지의 같은 본체/목록 저장소다.
    const SidPool& pool_; //기술/생산/시설의 현재 슬롯을 읽는다.
    std::span<const RiftTypeRecord> types_; //현재 비용이 아닌 원소/flags2 분류를 읽는다.
    const ContainedFinderState& finder_; //기존 현재 권한/containedType와 저장 next를 공유한다.
    AiFactoryProductionGlobals globals_; //현재 전역의 참조와 getter다.
    AiFactoryProductionHooks hooks_; //선택된 기존 실제 하위 효과다.
};
}
