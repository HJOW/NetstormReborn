// 실제 destination/source가 겹쳐도 원문 앞방향 DWORD 복사의 순서대로 처리한다.
#include "o/RawPacketCopy.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { //원문 포인터 폭과 호스트의 소유/물리 경계를 분리한다.
namespace { //선택된 BYTE/WORD/DWORD 접근 위치에서만 실제 물리를 검사한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("패킷 복제의 실제 읽기 물리 부족");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //같은 DWORD 안은 쓰기 전에 모두 읽어 원문 MOVSD의 중첩 의미를 유지한다.
void Write(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("패킷 복제의 실제 쓰기 물리 부족");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //이미 끝난 앞 DWORD/BYTE는 뒤 물리 오류로 되돌리지 않는다.
}
RawPacketCopyResult DuplicateRawPacket(std::span<std::uint8_t> packet,std::int32_t capacity,std::span<std::uint8_t> destination,RawPacketDispatch& dispatch) {
    if (!packet.data()) return {};const auto entry=dispatch.Registration(Read(packet,1,1));const auto pin=entry.Pin();entry.NotifyObserver(packet,1);const auto length=Read(packet,2,2);RawPacketCopyResult result; //원문의 현재 observer 슬롯과 뒤 WORD를 한 번씩 읽는다.
    if (!destination.data() && capacity==0) { result.allocation=std::make_shared<std::vector<std::uint8_t>>(length);destination=*result.allocation;result.present=true; }
    else { if (capacity<static_cast<std::int32_t>(length)) return {};result.present=destination.data()!=nullptr; }
    std::size_t offset{};for (std::uint32_t word=0;word<(length>>2);++word,offset+=4) Write(destination,offset,Read(packet,offset,4),4); //원문의 낮은 주소부터 DWORD 수만큼 복사하며 memmove로 바꾸지 않는다.
    for (std::uint32_t byte=0;byte<(length&3U);++byte,++offset) Write(destination,offset,Read(packet,offset,1),1); //남은0~3BYTE만 같은 방향으로 복사한다.
    result.bytes=destination.first(length);return result;
}
std::uint32_t RawPacketCopies::Hold(std::shared_ptr<std::vector<std::uint8_t>> allocation) {
    if (!allocation) throw std::logic_error("소유 패킷 복제의 실제 할당 누락");if (!nextToken_) throw std::overflow_error("소유 패킷 복제의 DWORD token 소진");const auto token=nextToken_;copies_.emplace(token,std::move(allocation));++nextToken_;return token; //장부 할당 실패로 앞선 observer/바이트 복사를 되돌리지 않는다.
}
std::uint32_t RawPacketCopies::Duplicate(std::span<std::uint8_t> packet,RawPacketDispatch& dispatch) { auto copy=DuplicateRawPacket(packet,0,{},dispatch);return copy.present ? Hold(std::move(copy.allocation)):0; } //원문 NULL 결과를 할당 성공으로 합성하지 않는다.
std::uint32_t RawPacketCopies::Adopt(std::vector<std::uint8_t> packet) { return Hold(std::make_shared<std::vector<std::uint8_t>>(std::move(packet))); } //완성된 일반 LAN Zone의 실제 byte 할당을 그대로 인계한다.
std::shared_ptr<std::vector<std::uint8_t>> RawPacketCopies::Acquire(std::uint32_t token) const { const auto found=copies_.find(token);if (!token || found==copies_.end()) throw std::out_of_range("소유 Zone의 현재 token 물리 누락");return found->second; } //현재 장부를 조회하며 이름/미션 Zone으로 대체하지 않는다.
void RawPacketCopies::Release(std::uint32_t token) { if (token && copies_.erase(token)!=1) throw std::out_of_range("소유 Zone의 현재 token 해제 누락/중복"); } //원문 free(NULL)의 무효과를 유지하고 실제 token은 한 번만 반납한다.
}
