// Squid의 소유자 지정(vtable +0x74)을 raw SID 풀과 소유자별 작업장 목록에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidPostPop.h"
#include <string_view>

namespace netstorm::o {
struct SquidOwnerDiagnostics { // 같은 World의 TYPE와 Client 진단을 선택 위치에서 빌린다.
    std::span<const RiftTypeRecord> types; // 빈 경우 독립 입력의 기존 생성 시 사본을 사용한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 식/파일/행과 정상 복귀를 전달한다.
};
// 소유자 지정이 읽는 원본 모드 전역이다. 패치 주소 / CD 주소를 괄호에 둔다.
struct SquidOwnerMode {
    bool fort{};      // 00594fb8 / CD 00540a1c: 요새(편집) 모드.
    bool battle{};    // 00594fbc / CD 00540a20: 전투 모드. 둘 중 하나가 켜져 있어야 작업장 목록을 고친다.
    bool challenge{}; // 00594fc8 / CD 00540a24: 켜져 있으면 유효한 소유자 번호가 1~39로 바뀐다(원본 CL_NUM_CHAL_PLAYERS).
};
// 소유 플레이어 변경을 객체 필드와 장부에 반영하는 처리기이다.
class SquidOwner {
public:
    // 풀·장부·모드는 이 객체보다 오래 살아야 한다. 타입 표는 복사한다.
    SquidOwner(SidPool& pool, std::span<const RiftTypeRecord> types, SquidPostPopState& bookkeeping, const SquidOwnerMode& mode);
    // 004adf00 ↔ CD 004aefa0: 객체의 소유자 바이트를 바꾼다. 요새/전투 모드에서 genus가 vortex·factory(0x4200)이면
    // 이전 소유자의 작업장 목록에서 이 번호를 모두 빼고, void·buried가 아니면 새 소유자의 목록 끝에 넣는다(가득 차면 넣지 않는다).
    // 소유자 0은 목록이 없다. 범위 밖 번호는 실제 원문 진단이 정상 복귀하면 하위 BYTE를 쓰며 목록 물리는 선택 시 확인한다.
    void Set(Sid sid, std::uint32_t player);
    void BindDiagnostics(SquidOwnerDiagnostics diagnostics); // 현재 TYPE 길이를 확인하고 실제 장치를 대여한다.
    // 가상 효과 어댑터가 같은 실제 풀의 소유자를 변경하는지 확인한다.
    const SidPool& Pool() const;
private:
    std::uint32_t TypeGenus(std::uint8_t type) const; // 선택된 위치에서 현재 TYPE의 물리 범위를 확인한다.
    SquidOwnerDiagnostics diagnostics_; // 참조한 TYPE/Client 장치는 이 객체보다 오래 살아야 한다.
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    // bookkeeping_: 객체 종류·소유자별 등록 목록과 개수를 관리하는 장부.
    SquidPostPopState& bookkeeping_;
    // mode_: 소유자 지정이 읽는 원본 모드 전역이다. 패치 주소 / CD 주소를 괄호에 둔다 (SquidOwnerMode).
    const SquidOwnerMode& mode_;
};
}
