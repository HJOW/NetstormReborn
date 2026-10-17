// Carrier의 일시적 내용물 그림을 만료 시 부모 삭제로 정리하는 10.78 프로세스다.
#pragma once
#include "o/SquidProcess.h"
#include <array>

namespace netstorm::o {
// 428700의 원문 24바이트다. 실제 호스트 포인터 대신 원본 VT 주소와 DWORD를 보관한다.
struct ContentPreviewFields {
    std::array<std::uint8_t,24> bytes{}; // +c 패딩을 포함해 생성자가 건드리지 않는 바이트도 보존한다.
    double Deadline() const; // +10의 현재 double 만료 시각이다.
    Sid Parent() const; // +8의 현재 부모 번호이며 호스트 연결은 실제 유효 SID만 받는다.
};
// 생성/삭제의 큰 경계다. 선택된 효과가 없으면 조용히 성공시키지 않고 오류를 낸다.
struct ContentPreviewHooks {
    std::function<void(ContentPreviewFields&,Sid,std::uint32_t,std::uint32_t)> attach; // 41bd20: 같은 필드/부모/현재 타입/flags10을 부착한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 만료한 부모의 현재 가상+10에 flags400000을 전달한다.
    std::function<void()> assertion; // 41bfb0의 parent->getHead()!=0 진단이며 반환하면 캡처한 부모를 계속 쓴다.
    std::function<void(ContentPreviewFields&)> release; // scalar destructor의 flags 하위 bit0이 선택한 원본 할당 해제 경계다.
};
// 428700/428770/480260/468230→41bd00의 생성·실행·통지·소멸 몸체다.
class RawContentPreviewProcess {
public:
    RawContentPreviewProcess(SidPool& pool,const double& now,const std::uint32_t& processType,ContentPreviewHooks hooks); // 시계/타입/풀은 로직보다 오래 살아야 한다.
    void Construct(ContentPreviewFields& fields,Sid parent,float duration) const; // duration float에 현재 double 시각을 더한 뒤 부착한다.
    void Run(ContentPreviewFields& fields) const; // now>=deadline일 때만 부모를 지우며 NaN 비교는 건너뛴다.
    static void Notify(ContentPreviewFields& fields,std::uint32_t event,std::uint32_t info,std::uint32_t value); // 실제 480260은 세 인자를 정리하는 빈 몸체다.
    void Destruct(ContentPreviewFields& fields,std::uint32_t flags) const; // base VT/두 진단 DWORD를 쓰고 하위 bit0이면 메모리를 해제한다.
    const SidPool& Pool() const; // 다른 Host의 부모/프로세스를 혼합하지 않게 검사한다.
private:
    SidPool& pool_; // 부모와 부착 Form이 공유하는 실제 풀이다.
    const double& now_; // DAT55b4d0의 현재 게임 시각이다.
    const std::uint32_t& processType_; // DAT54112c의 현재 값이며 SoundProcess와 같은 전역이다.
    ContentPreviewHooks hooks_; // 부모 삭제/부착/진단/할당 해제의 실제 경계다.
};
// 같은 Kernel과 ProcessForm 수명에 원문 필드를 연결한다. 실제 생성은 Construct의 attach 경계가 수행한다.
class ContentPreviewProcess final : public SquidProcess {
public:
    ContentPreviewProcess(const RawContentPreviewProcess& logic,ContentPreviewFields fields); // 생성자의 이미 계산된 필드를 보관한다.
    ~ContentPreviewProcess() override; // C++ 소유권 해제 직전에 원본 base 소멸 쓰기를 수행한다.
    void RunFrame() override; // 같은 Host에 부착한 프로세스만 실행하며 삭제 뒤 멤버를 다시 읽지 않는다.
    void OnAttached() override; // 실제 Host가 확정한 Form/부모를 두 DWORD에 쓴다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override; // 가상1c의 실제 빈 통지를 연결한다.
    ContentPreviewFields& Fields(); // 현재 단일 원문 필드를 반환한다.
private:
    const RawContentPreviewProcess& logic_; // 모든 프로세스보다 오래 사는 같은 월드 로직이다.
    ContentPreviewFields fields_; // Kernel 소유권과 함께 사라지는 유일한 필드다.
};
}
