// 최초 간헐천 등록에서 같은 현재 Player 원본 기록을 읽고 미복원 AI 하위 요청을 드러낸다.
#pragma once
#include "o/RiftType.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <functional>

namespace netstorm::o {
// nonnull AI를 빈 callback으로 처리하지 않도록 첫 실제 하위 요청의 인자를 보존하는 예외다.
class GeyserAiRegistrationBoundary final : public std::runtime_error {
public:
    // 첫 미복원 요청의 Player 번호·원본 AI 포인터·간헐천 SID·등록/제거 DWORD를 저장한다.
    GeyserAiRegistrationBoundary(std::uint32_t player,std::uint32_t pointer,
        std::uint32_t geyser,std::uint32_t operation);
    std::uint32_t owner; // 실제 순회에서 발견한 Player1..8 번호다.
    std::uint32_t aiPointer; // 현재 기록 +b0/CD+a8의 32비트 AI 포인터 값이다.
    std::uint32_t sid; // 원본 첫 인자이며 null AI 경로에서는 SID 유효성 검사를 하지 않는다.
    std::uint32_t state; // 원본 둘째 인자다. 0은 제거이고 nonzero는 등록 하위 요청이다.
};
// PATCH00416720/CD004d8670의 Player1..8 순회다. PATCH 실제 하위를 연결하면 현재 본체를 갱신한다.
class RawGeyserAiRegistration {
public:
    // 동일 Player 9기록을 복사하지 않고 참조한다. 호출 동안 해당 BYTE 저장소가 유효해야 한다.
    RawGeyserAiRegistration(OriginalEdition edition,std::span<const std::uint8_t> playerRecords);
    // 현재 AI 포인터를 번호순으로 읽고 연결된 하위를 호출한다. 미연결 nonnull이면 첫 요청을 명시 거부한다.
    void Register(std::uint32_t sid,std::uint32_t state) const;
    void BindPatch(std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> update); // 실제004153c0을 PATCH에서만 연결한다. 미연결/CD는 기존 첫 하위 경계를 보존한다.
private:
    std::span<const std::uint8_t> players_; // Money/Technology 등과 공유하는 실제 Player 저장소다.
    std::size_t stride_; // PATCH Player0xb4/CD Player0xac의 한 기록 크기다.
    std::size_t aiOffset_; // PATCH+b0/CD+a8의 현재 AI 포인터 위치다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> update_; // 현재 Player가 가리키는 본체·SID·원본 state를 같은 하위에 전달한다.
};
}
