// 현재 서버 시각/연결 정보와 실제 패킷을 구형 바이너리 순서로 기록한다.
#include "root/PacketRecord.h"
#include "root/ChallengeDetail.h"
#include "platform/Utf8Path.h"

namespace netstorm::root { // 실행 파일/원본 경로에 기록 파일을 만들지 않는다.
using namespace challenge_detail; // 같은 현재 주소/패킷의 지정 폭을 읽는다.
PacketRecord::PacketRecord(ClockState& clock,platform::UserDataRoot user):clock_(clock),user_(std::move(user)) {}
PacketRecord::~PacketRecord() { Close(); }
void PacketRecord::Close() { enabled_=0;if (file_) { std::fclose(file_);file_=nullptr; } }
void PacketRecord::Open(std::string_view name) {
    Close();const auto text=name.substr(0,name.find('\0'));const auto leaf=platform::PathFromUtf8(text).filename();if (leaf.empty() || leaf==L"." || leaf==L"..") throw std::invalid_argument("R 기록 파일의 실제 파일 이름 필요");platform::UserDataRoot::EnsureDirectory(user_.Diagnostics());enabled_=1;file_=::_wfopen((user_.Diagnostics()/leaf).c_str(),L"w+b");
} // 기존 옵션의 요청 이름은 보존하되 디렉토리는 사용자 Diagnostics로 이동한다.
void PacketRecord::Write(bool outgoing,const o::PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t length) {
    static_cast<void>(length);if (!enabled_) return;if (!file_) throw std::logic_error("R 기록 enabled1의 원문 NULL FILE 쓰기 거부");static_assert(std::endian::native==std::endian::little && sizeof(double)==8); // 지원 Windows x64/ARM64에서 구형 바이너리 필드의 실제 폭/바이트 순서다.
    const std::uint8_t direction=outgoing ? 1:0;const auto address=Read(connection.address,4);std::fwrite(&direction,1,1,file_);std::fwrite(&clock_.wall,8,1,file_);std::fwrite(&clock_.frameNumber,4,1,file_);std::fwrite(&address,4,1,file_);std::fwrite(&connection.sender,4,1,file_);const auto count=Read(packet,2,2);if (count>packet.size()) throw std::out_of_range("R 기록의 현재 패킷 선언/물리 길이 범위");std::fwrite(packet.data(),count,1,file_);std::fflush(file_); // 원문은 개별 fwrite/flush 결과로 이전 쓰기/현재 표를 되돌리지 않는다.
}
void PacketRecord::Bind(ConfigurationHooks& config,TransceiverHooks& transport) { config.openRecord=[this](auto name) { Open(name); };transport.record=[this](auto outgoing,auto& connection,auto packet,auto length) { Write(outgoing,connection,packet,length); }; }
}
