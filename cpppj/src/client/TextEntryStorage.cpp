// 편집 본체와 줄offset 배열의 재할당/반납을 실제 같은 물리에 연결한다.
#include "client/TextEntryStorage.h"
#include <algorithm>
#include <new>
#include <stdexcept>
namespace netstorm::client { // 실제 편집 저장소의 구현을 모으는 이름 공간이다.
// 선택 전에는 clipboard 장치를 요구하지 않는다.
TextEntryStorage::TextEntryStorage(std::function<void(std::span<char>)> read,std::function<void(std::span<const char>)> write):clipboardRead_(std::move(read)),clipboardWrite_(std::move(write)) {}
// 객체 주소는 map 삽입/새 객체 추가로 이동하지 않는다.
TextEntryBuffer& TextEntryStorage::Create(std::uint32_t token) {
    if (!token || buffers_.contains(token)) throw std::logic_error("편집 버퍼 객체 토큰 중복");auto value=std::make_unique<Record>();TextEntryBufferHooks hooks;
    hooks.allocate=[this](auto size) { return Allocate(size); };hooks.release=[this](auto address) { Release(address); };hooks.resize=[this](auto address,auto size) { return Resize(address,size); };hooks.bytes=[this](auto address) { return Bytes(address); };
    hooks.clipboardRead=[this](auto bytes) { if (!clipboardRead_) throw std::logic_error("실제 clipboard 읽기 미연결");clipboardRead_(bytes); };hooks.clipboardWrite=[this](auto bytes) { if (!clipboardWrite_) throw std::logic_error("실제 clipboard 쓰기 미연결");clipboardWrite_(bytes); };
    value->handler=std::make_unique<TextEntryBuffer>(value->state,std::move(hooks));auto& result=*value->handler;buffers_.emplace(token,std::move(value));return result;
}
// 현재 객체 자체와 본문의 수명을 구별한다.
TextEntryBuffer* TextEntryStorage::Find(std::uint32_t token) const { const auto found=buffers_.find(token);return found==buffers_.end() ? nullptr : found->second->handler.get(); }
// 생성 전/정상 Destroy 이후의 객체를 반납하며 살아 있는 본문을 잃지 않는다.
void TextEntryStorage::ReleaseBuffer(std::uint32_t token) { const auto found=buffers_.find(token);if (found==buffers_.end() || found->second->state.data || found->second->state.offsets) throw std::logic_error("편집 버퍼 본문 반납 전 객체 해제");buffers_.erase(found); }
// 원문 미정 바이트를 임의 sentinel/0으로 채우지 않는다.
std::uint32_t TextEntryStorage::Allocate(std::uint32_t size) {
    const auto step=(std::uint64_t(size)+31U)&~std::uint64_t(15U);if (std::uint64_t(nextAddress_)+step>=0x60000000ULL) throw std::overflow_error("편집 메모리 주소 범위 소진");
    std::unique_ptr<std::uint8_t[]> bytes(new(std::nothrow) std::uint8_t[size ? size : 1]);if (!bytes) return 0;const auto address=nextAddress_;nextAddress_+=static_cast<std::uint32_t>(step);blocks_.emplace(address,Block{std::move(bytes),size});return address;
}
// 내부 오프셋 포인터를 free로 잘못 사용하지 않는다.
void TextEntryStorage::Release(std::uint32_t address) { if (address && blocks_.erase(address)!=1) throw std::logic_error("편집 메모리 해제 주소 불일치"); }
// 실제 새 저장소가 성공한 뒤에만 원본을 반납한다.
std::uint32_t TextEntryStorage::Resize(std::uint32_t address,std::uint32_t size) {
    if (!address) return Allocate(size);const auto found=blocks_.find(address);if (found==blocks_.end()) throw std::logic_error("편집 메모리 재할당 주소 불일치");if (!size) { Release(address);return 0; }
    const auto next=Allocate(size);if (!next) return 0;std::copy_n(found->second.bytes.get(),std::min(size,found->second.size),blocks_.at(next).bytes.get());Release(address);return next;
}
// 같은 실제 할당의 끝도 빈 범위로 표현할 수 있다.
std::span<std::uint8_t> TextEntryStorage::Bytes(std::uint32_t address) const {
    auto found=blocks_.upper_bound(address);if (found==blocks_.begin()) throw std::out_of_range("편집 메모리 주소 없음");--found;const auto offset=address-found->first;if (offset>found->second.size) throw std::out_of_range("편집 메모리 오프셋 범위");return {found->second.bytes.get()+offset,found->second.size-offset};
}
// 게임 값을 바꾸지 않는 실제 소유 수 관측이다.
std::size_t TextEntryStorage::BufferCount() const { return buffers_.size(); }
// 문자/줄표/제외문자의 현재 할당 수다.
std::size_t TextEntryStorage::AllocationCount() const { return blocks_.size(); }
}
