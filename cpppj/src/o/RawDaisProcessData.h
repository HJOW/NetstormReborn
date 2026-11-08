// DaisProcess의 실제40BYTE 이미지·빈 실행·6ah 선택 decoder/writer를 같은 Kernel 수명에 연결한다.
#pragma once
#include "o/SquidProcess.h"
#include <array>
#include <span>

namespace netstorm::o { // 원문 주소는 기록값이며 C++ 가상 포인터나 PID로 변환하지 않는다.
// outer0/inner+10/dirty+18/기한+20의 원문40BYTE다. 미작성 padding의 기본0은 호스트 값이며 원문 초기값으로 주장하지 않는다.
struct DaisProcessFields {
    std::array<std::uint8_t,40> bytes{}; // caller가 제공한 padding은 생성자의 명시 쓰기 외에 보존한다.
};
// 0044a2d4..fb의 새 객체 쓰기만 수행하며 기존 Regular·희생 진행을 시작하지 않는다.
void InitializeDaisProcessFields(DaisProcessFields& fields);
// 0041bd00의 원문 base 소멸 표식이며 실제 C++ 메모리 해제/Kernel 소유권은 호출자에게 남긴다.
void DestructDaisProcessFields(DaisProcessFields& fields);
// 신규 수신이면 parent DWORD를 읽어 반환한다. 기존 수신은 parent 영역을 건너뛰며 bit2 저장 때만 fields가 필요하다.
std::uint32_t ReadDaisProcessData(DaisProcessFields* fields,std::span<const std::uint8_t> packet,bool newProcess);
// 004488c0은 parentPayload 비영일 때만 부모를 쓰고 현재 dirty2의8BYTE·길이·mask를 순서대로 쓴 뒤 dirty를 지운다.
std::uint16_t WriteDaisProcessData(DaisProcessFields& fields,std::span<std::uint8_t> packet,std::uint32_t form,std::uint32_t parentPayload,bool force,std::uint8_t protocol=1,std::uint8_t message=0x6a);
// 실제 Host/Kernel이 단독 소유하는 원문 DaisProcess다. RawDaisRegular와 다른 빈 RunFrame 객체다.
class DaisProcess final : public SquidProcess {
public:
    explicit DaisProcess(DaisProcessFields fields={}); // 주어진 같은 이미지에 원문 생성자 명시 필드만 쓴다.
    ~DaisProcess() override; // 원문 base 표식만 쓴다. 새 Form 삭제/Kernel 제거를 추가하지 않는다.
    void RunFrame() override; // outerVT+18=4437f0의 빈 실행이며 기한으로 사건을 만들지 않는다.
    void OnAttached() override; // 실제 Host의 현재 Form/부모를 이미지에 반영하는 호스트 표현 동기화다. 원문 Attached 통지는 비어 있다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override; // outer+1c/inner의480260은 빈 통지다.
    DaisProcessFields& Fields(); // 같은 원문 이미지의 dirty/기한을 decoder에 대여한다.
    const DaisProcessFields& Fields() const; // 정적 자료와 writer가 같은 이미지임을 읽기 전용으로 확인한다.
    std::uint16_t WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force=false,std::uint8_t protocol=1,std::uint8_t message=0x6a); // 4492f0의 현재 Host 필드와 parentChanged/force를 연결한다.
private:
    DaisProcessFields fields_; // C++ vptr와 별개인 기록 이미지이며 이 프로세스와 같은 수명이다.
};
}
