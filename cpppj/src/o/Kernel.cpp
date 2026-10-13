// 원본: 00471950(A) ↔ CD 004ed500, 004719e0(A) ↔ CD 004ed590, 00471a30(A) ↔ CD 004ed5d0.
// CD판의 슬롯 수 3,999를 패치판의 39,999로 바꾼 실제 판본 차이를 유지한다.
// 범위: 소유권·검색·프레임 호출. 원본 procSumm.txt 디버그 출력은 아직 복원하지 않았다.
#include "o/Kernel.h"
#include <stdexcept>

namespace netstorm::o {
// 슬롯 0을 포함하여 원본 패치판과 같은 번호 범위를 확보한다.
Kernel::Kernel(std::uint32_t capacity) : processes_(static_cast<std::size_t>(capacity)+1),owned_(processes_.size()),capacity_(capacity) {
    if (!capacity || capacity>kProcessCapacity) throw std::invalid_argument("Kernel 슬롯 크기 오류");
}
// 원본 Clear의 남은 앞 슬롯을 숨기지 않고 호스트 수명 종료 단계에서만 후속 정리를 수행한다.
Kernel::~Kernel() {
    // 모든 배열과 count가 살아 있을 때 소멸자 재진입의 조회/삭제/등록을 처리한다.
    do { Clear(); } while (count_);
}
// 원본처럼 첫 빈 슬롯을 기억하면서도 뒤에 있는 같은 포인터를 먼저 재사용한다.
ProcessId Kernel::Add(std::unique_ptr<BaseProcess> process) {
    if (!process) throw std::invalid_argument("Null process");
    ProcessId first=0;
    // 빈 앞 슬롯이 있어도 동일 포인터의 기존 뒤 슬롯까지 검색한다.
    for (ProcessId id=1;id<=capacity_;++id) {
        if (!processes_[id] && !first) first=id;
        if (processes_[id]==process.get()) { process.release();return id; }
    }
    if (!first) throw std::runtime_error("Out of processes");
    processes_[first]=process.get();owned_[first]=std::move(process);++count_;return first;
}
// 이미 소유한 객체는 새 소유권을 만들지 않고 원본의 중복 등록 검색만 실행한다.
ProcessId Kernel::Add(BaseProcess& process) {
    const auto id=Find(process);
    if (!id) throw std::invalid_argument("미등록 프로세스 참조의 소유권이 없습니다");
    return id;
}
// 원본 등록 함수가 이미 있는 프로세스의 번호를 반환하던 부분.
ProcessId Kernel::Find(const BaseProcess& process) const {
    // 포인터가 같은 슬롯을 앞에서부터 찾는다.
    for (ProcessId id=1;id<=capacity_;++id) if (processes_[id]==&process) return id;
    return 0;
}
// 슬롯 0은 미등록이므로 항상 null이다. 원본의 범위 밖 배열 읽기는 복제하지 않는다.
BaseProcess* Kernel::Get(ProcessId id) const { return id==0 || id>capacity_ ? nullptr : processes_[id]; }
// 원본처럼 제거 시 바로 소멸한다. 실행 중 자기 자신을 제거한 콜백은 멤버를 다시 사용하지 않아야 한다.
void Kernel::Remove(ProcessId id) {
    if (id==0 || id>capacity_ || !processes_[id]) throw std::out_of_range("Process not registered");
    // 가상 소멸자 안의 Get/Find/재등록은 아직 원래 슬롯을 관찰한다. 소멸 뒤에만 빈 슬롯을 만든다.
    owned_[id].reset();
    processes_[id]=nullptr;
    --count_;
}
// 현재 슬롯을 차례로 읽으므로 뒤 신규 등록은 같은 순회, 앞 신규 등록은 다음 명시 호출에 남는다.
void Kernel::Clear() {
    // 원본의 정확한 증가 순서를 유지하고 빈 슬롯은 건드리지 않는다.
    for (ProcessId id=1;id<=capacity_;++id) if (processes_[id]) Remove(id);
}
// 실행 중 뒤 슬롯에 등록된 프로세스는 같은 프레임에 실행되고, 이미 지난 슬롯이면 다음 프레임이다.
void Kernel::RunFrame() {
    // 원본의 즉시 슬롯 읽기를 유지한다. 목록을 복사하면 새 프로세스 실행 시점이 달라진다.
    for (ProcessId id=1;id<=capacity_;++id) if (processes_[id]) processes_[id]->RunFrame();
}
// 현재 등록된 프로세스 개수를 반환한다.
std::size_t Kernel::Size() const { return count_; }
// 판본 비교와 현재 슬롯 유효성 확인에 사용한다.
std::uint32_t Kernel::Capacity() const { return capacity_; }
}
