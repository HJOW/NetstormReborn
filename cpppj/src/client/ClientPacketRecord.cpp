// 구형21BYTE 기록 접두와 현재 패킷 WORD 길이를 실제 사용자 홈 파일에 작성한다.
#include "client/ClientPacketRecord.h"
#include "platform/Utf8Path.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { //설치/원본 폴더에 자동 파일을 만들지 않는다.
namespace { //현재 주소/패킷의 지정 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Client 패킷 기록의 실제 필드 물리 부족");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //원문 BYTE/WORD/DWORD의 낮은 BYTE부터 조립한다.
}
ClientPacketRecord::ClientPacketRecord(std::shared_ptr<o::FrameTime> frame,std::optional<platform::UserDataRoot> user):frame_(std::move(frame)),user_(std::move(user)) { if (!frame_) throw std::invalid_argument("Client 패킷 기록의 실제 프레임 owner 누락"); } //파일/폴더는 Open까지 지연한다.
ClientPacketRecord::~ClientPacketRecord() { Close(); } //현재 Client 기록만 반납한다.
void ClientPacketRecord::Close() { enabled_=0;if (file_) { std::fclose(file_);file_=nullptr; } } //닫기 실패로 요청/다른 전송 상태를 되돌리지 않는다.
void ClientPacketRecord::Open(std::string_view name) {
    Close();enabled_=1;const auto leaf=platform::PathFromUtf8(name.substr(0,name.find('\0'))).filename();if (leaf.empty() || leaf==L"." || leaf==L"..") throw std::invalid_argument("Client 패킷 기록 파일 이름 누락");const auto user=user_ ? *user_:platform::UserDataRoot::Resolve();platform::UserDataRoot::EnsureDirectory(user.Diagnostics());file_=::_wfopen((user.Diagnostics()/leaf).c_str(),L"w+b"); //원문 파일 이름을 유지하고 사용자 홈 아래에만 쓴다. fopen 실패의NULL/enable1은 보존한다.
}
void ClientPacketRecord::Write(bool outgoing,const o::PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t length) {
    const auto address=Read(connection.address,4,4),sender=connection.sender;static_cast<void>(length);if (!enabled_) return;if (!file_) throw std::logic_error("Client 기록 enabled1의 원문 NULL FILE 쓰기 거부");static_assert(std::endian::native==std::endian::little && sizeof(double)==8); //실제 원문21BYTE 접두의 폭/바이트 순서를 Windows x64/ARM64에서도 유지한다.
    const std::uint8_t direction=outgoing ? 1:0;std::fwrite(&direction,1,1,file_);std::fwrite(&frame_->wall,8,1,file_);const auto number=static_cast<std::uint32_t>(frame_->number);std::fwrite(&number,4,1,file_);std::fwrite(&address,4,1,file_);std::fwrite(&sender,4,1,file_);
    const auto count=Read(packet,2,2);if (count>packet.size()) throw std::out_of_range("Client 기록의 현재 선언 길이/물리 범위");std::fwrite(packet.data(),count,1,file_);std::fflush(file_); //현재 본문 길이는 접두 작성 뒤 읽는다. 원문처럼 각 fwrite/flush 반환값으로 앞선 부분 효과를 되돌리지 않는다.
}
}
