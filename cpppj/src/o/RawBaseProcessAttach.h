// BaseProcess의 로컬 신규 생성과 지정 Form 분기를 같은 원문 부모로 표현한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 원문 필드/현재 번호와 선택 장치를 빌리는 부착 부모다.
struct BaseProcessAttachFields { // C++ 객체의 실제 두 SID 필드이며 callback 뒤에도 현재 값을 읽는다.
    Sid& form; // 원문 this+4의 호스트 SID 표현이다.
    Sid& parent; // 원문 this+8의 호스트 SID 표현이다.
};
struct BaseProcessAttachHooks { // 실패 시 이미 쓴 필드/등록/슬롯을 복구하지 않는다.
    std::function<std::uint32_t()> typeEnd; // 최초 타입 진단이 읽는 현재 signed 상한 DWORD다.
    std::function<std::uint32_t()> processFormType; // 신규 Create/타입 변경이 읽는 현재 번호다.
    std::function<std::uint32_t()> registerProcess; // 필드/부모 상태 뒤 Kernel에 등록하고 PID를 반환한다.
    std::function<Sid(std::uint32_t)> create; // 원문 할당 flags0/2로 새 Form을 만든다.
    std::function<void(Sid)> take; // 지정 free 슬롯의 현재 공통 Take다. 반환 객체는 부모가 사용하지 않는다.
    std::function<Sid(Sid)> parent; // 지정 슬롯의 현재 VT+0 부모 getter다.
    std::function<void(Sid,Sid,std::uint32_t)> attach; // 현재 VT+74를 선택한다.
    std::function<void()> onAttached; // 생성/지정 검증 뒤의 실제 프로세스 가상 통지다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 진단이 정상 복귀하면 원문 다음 위치로 진행한다.
};
bool AttachBaseProcess(SidPool& pool,BaseProcessAttachFields fields,std::uint32_t processType,Sid parent,Sid form,std::uint32_t flags,const BaseProcessAttachHooks& hooks); // PATCH 비활성 부모 조기 반환만 false다. 전체 원문 ABI 반환값은 아니다.
}
