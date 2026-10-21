// 원본 AI가 기존 월드와 간헐천을 같은 본체 목록에 등록하는 몸체다.
#pragma once
#include "o/RawAiObjectNotice.h"

namespace netstorm::o {
// 원본 중심점·로그·유효 표면 진단만 실제 세계에 요청한다.
struct AiWorldRegistrationHooks {
    std::function<std::array<float,2>(Sid)> center; //004adea0의 현재 신전 발 중심이다. 자신의 신전0일 때는 본체268/26c를 쓴다.
    std::function<void(std::uint32_t,std::uint32_t)> fartherGeyser; //004105b0의 Not closer to geyser 로그이며 본문이 후보를 추가하지 않은 뒤 호출한다.
    std::function<void(std::uint32_t)> missingSurface; //00411000의 좌표/표면0 진단이다. 반환하면 원본처럼 SID0 분류를 계속한다.
};
//00415720/004153c0의 실제0xb08·Player9·방향 동맹·표면 지도다. 별도 관찰용 목록을 만들지 않는다.
class RawAiWorldRegistration {
public:
    RawAiWorldRegistration(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const std::uint8_t> players,std::span<const std::uint32_t> allies,std::span<const std::uint16_t> surface,
        const RawAiObjectNotice& objects,AiWorldRegistrationHooks hooks); // 같은 Pool/본체/현재65536WORD를 빌린다.
    void FullScan(std::uint32_t token) const; //00415720: SID2부터 현재 live 항목→방어 자산 최대300개 지연→마지막3c8=3이다.25c는 caller가 쓴다.
    void Geyser(std::uint32_t token,std::uint32_t sid,std::uint32_t state) const; //004153c0:0은 다섯 목록 제거, 비영은 가까움/표면 소유에 따라 분류한다.
    bool CloserToAlly(std::uint32_t token,std::uint32_t sid) const; //004152b0의 엄격한 거리 비교이며 self 거리만 float에 저장한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; // 실제 살아 있는0xb08만 읽는다.
    Sid Object(std::uint32_t sid) const; // DWORD 번호를 실제 Pool 범위와16비트 계약으로 확인한다.0도 물리 슬롯으로 허용한다.
    std::uint32_t Genus(Sid sid) const; // 현재 TYPE의 순수 flags2다. 추가 분류 비트를 합성하지 않는다.
    std::int32_t Count(std::uint32_t token,std::size_t offset) const; // 현재 signed 목록 count다.
    void Append(std::uint32_t token,std::size_t offset,std::uint32_t sid) const; // full이면 포인터를 읽지 않고 반환하며 일반 중복은 유지한다.
    void Remove(std::uint32_t token,std::size_t offset,std::uint32_t sid) const; // 모든 중복을 안정 압축하고 inactive 꼬리를 보존한다.
    void ClassifySurface(std::uint32_t token,std::uint32_t sid) const; //00411000의 X/Y 절삭→표면·owner→330/348 분류다.
    RawAiBrainMemory& memory_; // 생성자/통지/전략과 공유하는 유일한 본체/목록 할당 소유자다.
    const SidPool& pool_; // 현재 live/free/type/좌표의 실제 풀이다.
    std::span<const RiftTypeRecord> types_; // 같은 현재188개 TYPE다.
    std::span<const std::uint8_t> players_; // 같은9×180BYTE의 현재 신전 기록이다.
    std::span<const std::uint32_t> allies_; // 방향별81DWORD이며 owner행을 호출 시 읽는다.
    std::span<const std::uint16_t> surface_; //005c84bc의 같은 hash0 원문이다.
    const RawAiObjectNotice& objects_; // 지연한 방어 자산도 같은00414de0으로 통지한다.
    AiWorldRegistrationHooks hooks_; // 선택 위치의 실제 중심/장치/진단 경계다.
};
}
