// 원본00471890의 현재 Kernel 슬롯 순서와 procSumm.txt 바이트 출력을 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 진단은 프레임을 실행하거나 프로세스/부모를 변경하지 않는다.
struct RiftTypeRecord; // 현재 부모 TYPE의 이름만 같은 표에서 빌린다.
// 32비트 원본 포인터와 64비트 호스트 객체를 동일한 불투명 식별자로 전달한다.
using ProcessSummaryHandle=std::uintptr_t;
// 순회와 서식은 원문이며 현재 프로세스 필드/파일 장치만 외부에서 공급한다.
struct ProcessSummaryHooks {
    std::function<ProcessSummaryHandle(std::uint32_t)> slot; // Kernel의 현재 슬롯이며0은 빈 슬롯이다.
    std::function<std::uint32_t(ProcessSummaryHandle)> form; // 현재 +4의 Form SID이며 uint16로 미리 자르지 않는다.
    std::function<std::int32_t(ProcessSummaryHandle)> parent; // 현재 +8의 signed 부모 SID다.
    std::function<ProcessSummaryHandle(std::string_view,std::string_view)> open; // fopen("procSumm.txt","wb")의 반환값이다.
    std::function<void(ProcessSummaryHandle,std::string_view)> write; // fprintf가 받는 전체 한 줄이며 LF를 변환하지 않는다.
    std::function<void(ProcessSummaryHandle)> close; // 빈 표/NULL 토큰도 원문처럼 같은 fclose 경계로 전달한다.
};
//0049a830의 전체 이름은 TYPE의20바이트 축약 이름과 구별한다.
std::string_view ProcessSummaryTypeName(std::uint32_t type);
//PATCH의39999슬롯을 실제 진단 순서로 읽는다. 호출 자체는 자체 파일 경로나 상태를 보유하지 않는다.
void WriteRawProcessSummary(const SidPool& pool,std::span<const RiftTypeRecord> types,const ProcessSummaryHooks& hooks);
}
