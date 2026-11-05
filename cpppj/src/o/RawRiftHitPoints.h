// TYPE 자체의 최대HP 조회와 같은 물리 표에서의 현재 번호 계산을 공유한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/RiftType.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // PATCH0049fce0/CD00444720의 현재 전역/선택된 TYPE를 구별한다.
struct RiftHitPointGlobals { // 진단 복귀 뒤에도 같은 주소에서 현재 값을 읽는다.
    RawCheckingFlag checking,quarter; // PATCH005e4794와005ca8f0/CD00511c90의 비영 관문이다.
    const std::uint32_t& priest; // PATCH005412d0/CD0051cbbc의 현재 특별 TYPE다.
    const std::uint32_t& dais; // PATCH005412ec의 HP 플래그 예외이며 CD는 읽지 않는다.
};
struct RiftHitPointHooks { // 호출 위치에서 선택된 실제 장치만 요구한다.
    std::function<std::uint32_t()> number; // 같은 this와 그때 현재 TYPE base로 계산한 getNum DWORD다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문1e3의 식/파일/줄이며 정상 복귀하면 계속한다.
};
// HP 플래그/현재 dais 진단 뒤 현재 약화/번호를 다시 읽고 처음 선택한 TYPE의 현재HP를 반환한다.
std::int32_t ReadRiftHitPoints(OriginalEdition edition,const RiftTypeRecord& type,const RiftHitPointGlobals& globals,const RiftHitPointHooks& hooks);
// 같은 물리 Record 배열을 옮겨 빌린 두 뷰의 상대 번호다. 음수도 DWORD 비트로 보존하며 다른 할당 영역은 거부한다.
std::uint32_t CurrentRiftTypeNumber(const RiftTypeRecord& type,std::span<const RiftTypeRecord> selectedBase,std::span<const RiftTypeRecord> currentBase);
}
