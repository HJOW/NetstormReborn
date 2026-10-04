// 원본 Kernel.cpp의 등록·삭제·슬롯 순서 실행. 패치판의 확장된 39,999 슬롯을 기준으로 한다.
#pragma once
#include "o/BaseProcess.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace netstorm::o {
// 패치 FUN_00471950: 1..39999. CD FUN_004ed500은 1..3999이며 서로 다른 값이다.
inline constexpr std::uint32_t kProcessCapacity = 39999;
// 원본 슬롯 0은 미등록을 뜻하며 실행하지 않는다.
using ProcessId = std::uint32_t;

class Kernel {
public:
    // 슬롯 배열을 힙에 둔다. 64비트 포인터로 바뀌어도 저장 파일 형식에는 영향이 없다.
    Kernel();
    // 프로세스 소유권을 커널에 넘기고 앞에서부터 첫 빈 슬롯을 배정한다.
    ProcessId Add(std::unique_ptr<BaseProcess> process);
    // 원본의 이미 등록된 포인터 검색을 별도 안전 API로 제공한다. 없으면 0.
    ProcessId Find(const BaseProcess& process) const;
    // 원본 004719e0처럼 가상 소멸자를 호출하고 슬롯을 비운다.
    void Remove(ProcessId id);
    // 원본 00471a30: 그 순간의 슬롯을 직접 읽으며 1..39999를 순회한다.
    void RunFrame();
    // 원본 디버그용 프로세스 개수와 같은 현재 등록 수.
    std::size_t Size() const;
private:
    std::vector<std::unique_ptr<BaseProcess>> processes_;
    std::size_t count_{};
};
}
