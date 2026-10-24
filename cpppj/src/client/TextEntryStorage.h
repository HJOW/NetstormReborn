// 편집 버퍼의 실제 미초기화 할당과32비트 주소+오프셋 조회를 공통 UI 수명에 소유한다.
#pragma once
#include "client/TextEntryBuffer.h"
#include <map>
#include <memory>
namespace netstorm::client { // 실제 Client 문자 편집 저장소를 모으는 이름 공간이다.
// 원문 편집 버퍼와 메모리 주소를 고정 소유자로 유지한다.
class TextEntryStorage {
public:
    TextEntryStorage(std::function<void(std::span<char>)> read,std::function<void(std::span<const char>)> write); // clipboard는 실제 선택된 입력 분기에서만 호출한다.
    TextEntryBuffer& Create(std::uint32_t token); // 같은 고정 상태/처리기를 새0x28 객체 토큰에 구성한다.
    TextEntryBuffer* Find(std::uint32_t token) const; // 살아 있는 실제 버퍼만 반환한다.
    void ReleaseBuffer(std::uint32_t token); // 버퍼 본문 Destroy 뒤 객체 기록 자체를 반납한다.
    std::uint32_t Allocate(std::uint32_t bytes); // 실제 new[]를0초기화 없이 확보한다. OOM은0이다.
    void Release(std::uint32_t address); // 같은 할당의 시작만 free하며 null은 허용한다.
    std::uint32_t Resize(std::uint32_t address,std::uint32_t bytes); // 성공 시 실제 내용을 보존해 이동하고 실패 시 원래 할당을 유지한다.
    std::span<std::uint8_t> Bytes(std::uint32_t address) const; // 같은 물리에서 주소+오프셋 뒤 남은 범위를 빌린다.
    std::size_t BufferCount() const;std::size_t AllocationCount() const; // 실제 소유 기록의 읽기 전용 수다.
private:
    struct Block { std::unique_ptr<std::uint8_t[]> bytes;std::uint32_t size{}; }; // 실제 미초기화 바이트와 요청 폭이다.
    struct Record { TextEntryBufferState state;std::unique_ptr<TextEntryBuffer> handler; }; // handler보다 오래 유지하는 실제0x28 논리 필드다.
    std::map<std::uint32_t,Block> blocks_;std::map<std::uint32_t,std::unique_ptr<Record>> buffers_; // 주소와 객체 토큰을 구별한다.
    std::uint32_t nextAddress_{0x20000000U}; // 포인터를 자르지 않고 주소 산술을 허용하는 단조 증가 가상 범위다.
    std::function<void(std::span<char>)> clipboardRead_;std::function<void(std::span<const char>)> clipboardWrite_; // 실제 장치 연결을 소유한다.
};
}
