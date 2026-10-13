// 원본 Kernel.cpp의 등록·삭제·슬롯 순서 실행. 패치판의 확장된 39,999 슬롯을 기준으로 한다.
#pragma once
#include "o/BaseProcess.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace netstorm::o {
// 패치 FUN_00471950: 1..39999. CD FUN_004ed500은 1..3999이며 서로 다른 값이다.
inline constexpr std::uint32_t kProcessCapacity = 39999;
// CD 10.72의 실제 Kernel 끝 번호다. 판본 비교 장면에서 생성자 인자로 사용한다.
inline constexpr std::uint32_t kCdProcessCapacity = 3999;
// 원본 슬롯 0은 미등록을 뜻하며 실행하지 않는다.
using ProcessId = std::uint32_t;

// 원본 슬롯 포인터와 단일 C++ 소유권을 함께 유지하는 프로세스 실행기다.
class Kernel {
public:
    // 슬롯 배열을 힙에 둔다. 64비트 포인터로 바뀌어도 저장 파일 형식에는 영향이 없다.
    explicit Kernel(std::uint32_t capacity=kProcessCapacity);
    // 호스트 수명 종료에는 남은 소유권까지 정리한다. 소멸자 콜백의 신규 생성은 유한하게 끝나야 한다.
    // Clear의 단일 순회 뒤 남은 앞 슬롯은 별도 후속 순회로 정리하며 원본 Clear 결과와 구분한다.
    ~Kernel();
    // 프로세스 소유권을 커널에 넘기고 앞에서부터 첫 빈 슬롯을 배정한다.
    ProcessId Add(std::unique_ptr<BaseProcess> process);
    // 이 Kernel이 이미 소유한 같은 포인터를 다시 등록하고 기존 번호를 반환한다.
    // 미등록 참조는 소유권을 넘길 수 없으므로 거부한다. 신규 객체는 unique_ptr 오버로드를 사용한다.
    ProcessId Add(BaseProcess& process);
    // 원본의 이미 등록된 포인터 검색을 별도 안전 API로 제공한다. 없으면 0.
    ProcessId Find(const BaseProcess& process) const;
    // 원본 00471aa0 ↔ CD 004ed640: 슬롯의 프로세스를 그대로 읽는다. 빈 슬롯과 범위 밖 번호는 null이다.
    BaseProcess* Get(ProcessId id) const;
    // 원본 004719e0처럼 가상 소멸자를 호출하고 슬롯을 비운다.
    void Remove(ProcessId id);
    // 00471af0/CD 004ed4d0: 슬롯 1부터 끝까지 한 번만 읽고 소멸자 완료 뒤 현재 슬롯을 비운다.
    // 소멸자가 이미 지난 빈 슬롯에 새 객체를 넣으면 이번 Clear에서는 남는다.
    void Clear();
    // 원본 00471a30: 그 순간의 슬롯을 직접 읽으며 1..39999를 순회한다.
    void RunFrame();
    // 원본 디버그용 프로세스 개수와 같은 현재 등록 수.
    std::size_t Size() const;
    // 판본별 마지막 등록 번호를 반환한다. 슬롯 0은 이 범위에 포함하지 않는다.
    std::uint32_t Capacity() const;
private:
    std::vector<BaseProcess*> processes_; // 원본처럼 소멸자 실행이 끝날 때까지 슬롯 포인터가 남는다.
    std::vector<std::unique_ptr<BaseProcess>> owned_; // 같은 포인터를 재등록해도 소유권은 한 슬롯에만 둔다.
    std::uint32_t capacity_{}; // 기본은 10.78의 39999, CD 비교는 3999다.
    std::size_t count_{};
};
}
