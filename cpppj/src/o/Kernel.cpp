// 원본: 00471950(A) ↔ CD 004ed500, 004719e0(A) ↔ CD 004ed590, 00471a30(A) ↔ CD 004ed5d0.
// CD판의 슬롯 수 3,999를 패치판의 39,999로 바꾼 실제 판본 차이를 유지한다.
// 범위: 소유권·검색·프레임 호출. 원본 procSumm.txt 디버그 출력은 아직 복원하지 않았다.
#include "o/Kernel.h"
#include <stdexcept>

namespace netstorm::o {
// 슬롯 0을 포함하여 원본 패치판과 같은 번호 범위를 확보한다.
Kernel::Kernel() : processes_(static_cast<std::size_t>(kProcessCapacity) + 1) {}
// 원본 첫 빈 슬롯 검색. 포인터 중복은 소유권 API에서 허용하지 않는다.
ProcessId Kernel::Add(std::unique_ptr<BaseProcess> process) {
    if (!process) throw std::invalid_argument("Null process");
    // 빈 슬롯을 찾으면 즉시 채운다. 원본의 중복 포인터 검색은 Find로 분리했다.
    for (ProcessId id = 1; id <= kProcessCapacity; ++id) {
        if (!processes_[id]) { processes_[id] = std::move(process); ++count_; return id; }
    }
    throw std::runtime_error("Out of processes");
}
// 원본 등록 함수가 이미 있는 프로세스의 번호를 반환하던 부분.
ProcessId Kernel::Find(const BaseProcess& process) const {
    // 포인터가 같은 슬롯을 앞에서부터 찾는다.
    for (ProcessId id = 1; id <= kProcessCapacity; ++id) if (processes_[id].get() == &process) return id;
    return 0;
}
// 원본처럼 제거 시 바로 소멸한다. 실행 중 자기 자신을 제거한 콜백은 멤버를 다시 사용하지 않아야 한다.
void Kernel::Remove(ProcessId id) {
    if (id == 0 || id > kProcessCapacity || !processes_[id]) throw std::out_of_range("Process not registered");
    processes_[id].reset();
    --count_;
}
// 실행 중 뒤 슬롯에 등록된 프로세스는 같은 프레임에 실행되고, 이미 지난 슬롯이면 다음 프레임이다.
void Kernel::RunFrame() {
    // 원본의 즉시 슬롯 읽기를 유지한다. 목록을 복사하면 새 프로세스 실행 시점이 달라진다.
    for (ProcessId id = 1; id <= kProcessCapacity; ++id) if (processes_[id]) processes_[id]->RunFrame();
}
// 현재 등록된 프로세스 개수를 반환한다.
std::size_t Kernel::Size() const { return count_; }
}
