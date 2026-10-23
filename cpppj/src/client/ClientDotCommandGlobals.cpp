// 정적 say 함수 값과 표시 주소는 원본 식별자이며 host 함수 포인터로 실행하지 않는다.
#include "client/ClientDotCommandGlobals.h"

namespace netstorm::client {
namespace {
// 고정 PE 정적 DWORD를 원래 little endian 순서로 같은 표에 쓴다.
void InitialDword(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) { // DWORD의 네 바이트만 초기화하며 인접 항목을 건드리지 않는다.
        bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
    }
}
}
// Native 함수와 별도로 선고정한 PE 전체 정적 이미지의 비영 바이트를 구성한다.
ClientDotCommandGlobals::ClientDotCommandGlobals() {
    bytes_[0]='s';bytes_[1]='a';bytes_[2]='y';
    InitialDword(bytes_,0x800,0x0042f1e0); // 최초 say 명령의 실제 원문 함수 식별자다.
    InitialDword(bytes_,0x900,kDotCommandNamesToken); // 표시0은 이름표 첫 슬롯을 가리킨다.
    InitialDword(bytes_,0xa04,1); // 등록은 say 다음의 슬롯1에서 시작한다.
}
// 실제 단일 표를 빌리며 Session/호출 교체에 따라 다시 초기화하지 않는다.
std::span<std::uint8_t> ClientDotCommandGlobals::Bytes() { return bytes_; }
// const 조회도 동일한 물리 배열을 반환한다.
std::span<const std::uint8_t> ClientDotCommandGlobals::Bytes() const { return bytes_; }
// 복사 callback 뒤 단일 count 캡처와 부분효과는 검증된 원문 등록 부모 자체가 수행한다.
std::uint32_t ClientDotCommandGlobals::Register(std::span<const std::uint8_t> name,std::uint32_t callback,std::uint32_t flag,const DotCommandRegisterHooks& hooks) {
    return RegisterDotCommand(bytes_,name,callback,flag,hooks);
}
}
