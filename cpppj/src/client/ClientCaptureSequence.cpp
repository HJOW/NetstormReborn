// 원문 본체가 현재 장치를 소비하게 연결하며 별도 근사 표면을 만들지 않는다.
#include "client/ClientCaptureSequence.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "client/IndexedBitmapFile.h"
#include "client/UberGump.h"
#include "o/RawBaseFileReadPolicy.h"
#include "o/OriginalText.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <cstdio>
#include <limits>
#include <stdexcept>
namespace netstorm::client { //원문 포인터표와 호스트 주소 폭을 토큰으로 구별한다.
std::string CaptureNoticeFontBytes(std::string_view text) {
    std::string result;
    for (std::size_t offset=0;offset<text.size();) { //유효한UTF-8문자하나씩만원문글꼴로변환한다.
        const auto start=offset;const auto lead=static_cast<std::uint8_t>(text[offset++]);const auto width=lead<0x80 ? 1U:lead>=0xc2 && lead<=0xdf ? 2U:lead>=0xe0 && lead<=0xef ? 3U:lead>=0xf0 && lead<=0xf4 ? 4U:0U;
        if (!width || start+width>text.size()) throw std::invalid_argument("Capture 안내의 UTF-8 경계");std::uint32_t code=lead&(width==1 ? 0x7fU:width==2 ? 0x1fU:width==3 ? 0x0fU:0x07U);
        for (unsigned index=1;index<width;++index) { const auto next=static_cast<std::uint8_t>(text[offset++]);if ((next&0xc0U)!=0x80U) throw std::invalid_argument("Capture 안내의 UTF-8 후속 바이트");code=(code<<6)|(next&0x3fU); } //같은코드포인트의전체바이트를소비한다.
        if ((width==2 && code<0x80) || (width==3 && code<0x800) || (width==4 && code<0x10000) || code>0x10ffff || (code>=0xd800 && code<=0xdfff)) throw std::invalid_argument("Capture 안내의 UTF-8 코드포인트");
        try { const auto bytes=o::EncodeOriginalText(text.substr(start,width));result.append(bytes.begin(),bytes.end()); }
        catch (const std::invalid_argument&) { char escaped[16]{};std::snprintf(escaped,sizeof(escaped),"[U+%04X]",static_cast<unsigned>(code));result+=escaped; } //Windows10/11 호환 차이: 원본8비트글꼴의미지원경로문자는식별표기로보여종료예외를막는다. Unicode실제파일명/원문서유럽문자는바꾸지않고한국어UI는6차에구현한다.
    }
    return result;
} //잘못된UTF-8자체는원문글꼴누락과구별해거부한다.
std::filesystem::path ResolveCaptureBitmapPath(const std::filesystem::path& root,std::string_view name,o::RawFileName* names) {
    name=name.substr(0,name.find('\0'));std::string slashes(name);std::replace(slashes.begin(),slashes.end(),'/','\\');
    const bool device=names ? !o::ReadBaseFileDevice(name,*names).empty() : name.find(':')!=name.npos || (name.find('\\')!=name.npos && slashes.starts_with("\\\\")); // 실제459900은 빈 장치 반환도 같은 고리 한 칸을 사용한다.
    auto path=platform::PathFromUtf8(device ? slashes:names ? o::JoinBaseFileRoot(platform::PathToUtf8(root),name,*names):o::JoinBaseFileRoot(platform::PathToUtf8(root),name)); //Windows10/11 호환: 원본ANSI 대신 UTF-8/W 경로다. 상대/장치 분기는 유지한다.
#ifdef _WIN32
    const auto drive=path.root_name();if (device && !path.has_root_directory() && drive.native().size()==2 && drive.native()[1]==L':' && o::AsciiLower(platform::PathToUtf8(drive))==o::AsciiLower(platform::PathToUtf8(root.root_name()))) path=root/path.relative_path(); //같은 드라이브 상대 이름은 게임CWD이며 공통 BaseFile과 같은 경계다.
#endif
    return path;
}
ClientCaptureSequence::ClientCaptureSequence(Client& client):client_(client) {} //장치 없이 같은 현재 Client만 참조한다.
const CaptureSequenceState& ClientCaptureSequence::State() const { return state_; } //새 상태로 복제하지 않는다.
std::size_t ClientCaptureSequence::AllocatedFrames() const { return allocations_.size(); } //검사/진단용 순수 조회다.
void ClientCaptureSequence::Start() { StartCaptureSequence(state_); } //파일/화면은 요구하지 않는다.
void ClientCaptureSequence::ReadScreen() {
    const auto& screen=client_.GetScreen();screen_.width=static_cast<std::uint32_t>(screen.Width());screen_.height=static_cast<std::uint32_t>(screen.Height());screen_.pitch=static_cast<std::uint32_t>(screen.Pitch());screen_.pixels=1;screen_.palette=2;
} //Stop의 현재 논리 크기를 실제 Screen에서 빌린다.
std::span<std::uint8_t> ClientCaptureSequence::Memory(std::uint32_t token,std::uint32_t length) {
    if (token==1) { if (length>pixels_.size()) throw std::out_of_range("Client 연속 저장의 현재 잠금 물리");return pixels_.first(length); }
    const auto& allocation=allocations_.at(token);if (length>allocation.size) throw std::out_of_range("Client 연속 저장의 실제 프레임 할당 범위");return {allocation.bytes.get(),length};
} //범위 밖 주소를 가짜 바이트로 늘리지 않는다.
CaptureSequenceHooks ClientCaptureSequence::Hooks() {
    CaptureSequenceHooks hooks;
    hooks.lock=[this] { auto* pixels=client_.GetScreen().Lock();ReadScreen();pixels_={pixels,static_cast<std::size_t>(screen_.height)*screen_.pitch}; };
    hooks.unlock=[this] { client_.GetScreen().Unlock();pixels_={}; };
    hooks.allocate=[this](auto size) { if (next_==std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Client 연속 저장의 할당 토큰 소진");const auto token=next_++;allocations_.emplace(token,Allocation{std::make_unique_for_overwrite<std::uint8_t[]>(size),size});return token; }; //전체pitch의 별도 실제 할당을0채움 없이 소유한다.
    hooks.memory=[this](auto token,auto length) { return Memory(token,length); };
    hooks.release=[this](auto token) { if (token && !allocations_.erase(token)) throw std::out_of_range("Client 연속 저장의 미소유 프레임 반납"); };
    hooks.prefix=[this] { return client_.Configuration().PathSpec("GifSequenceName"); }; //index인자를 붙이지 않는다.
    hooks.bitmap=[this](auto name,auto token,auto x,auto y,auto width,auto height,auto pitch,auto palette) {
        if (palette!=2) throw std::out_of_range("Client 연속 저장의 현재 논리 팔레트 토큰");const auto logical=client_.GetScreen().LogicalPalette();const auto& allocation=allocations_.at(token);IndexedBitmapScratch scratch; //지역 기본0을 원문 미정 스택의 값으로 주장하지 않는다.
        const auto& userData=client_.UserData(); // 5차: 연속 저장 BMP도 설치 폴더가 아닌 사용자 Captures에 쓴다(장치/절대 이름은 명시 위치 유지).
        if (userData) platform::UserDataRoot::EnsureDirectory(userData->Captures());
        WriteIndexedBitmapFile(ResolveCaptureBitmapPath(userData ? userData->Captures() : client_.GameDirectory(),name,&client_.StaticStrings()->FileNames()),{allocation.bytes.get(),allocation.size},{x,y,width,height,pitch},{reinterpret_cast<const std::uint8_t*>(logical.data()),logical.size_bytes()},scratch);
    };
    hooks.report=[this](auto first,auto last) { const auto bytes=client_.StaticStrings()->FormatCaptureRange(first,last);return std::string(reinterpret_cast<const char*>(bytes.data())); }; //공유 고리80예약/전체 서식 뒤 같은 본문을 즉시 전달한다.
    hooks.notice=[this](auto message,auto width,auto height) { auto* menu=client_.Menu();if (!menu) throw std::logic_error("Client 연속 저장의 현재 공통 HTML 미연결");menu->ShowCaptureNotice(message,width,height); };
    return hooks;
} //경로 선택과 실제 장치 실패를 빈 성공으로 바꾸지 않는다.
void ClientCaptureSequence::Frame() { if (state_.active) AppendCaptureSequenceFrame(state_,screen_,Hooks()); } //renderer994b0 뒤·후속 출력 앞에서만 Client가 호출한다.
void ClientCaptureSequence::Stop() {
    if (state_.count>0 && state_.frames[0]) ReadScreen(); //파일 출력 때 현재 크기를 동기화한다. 이 어댑터의PathSpec/CRT는 Screen을 바꾸지 않으며 현재값 변이는 별도 Raw 검사로 대조한다.
    StopCaptureSequence(state_,screen_,Hooks());
} //빈 수집 종료는 화면을 요구하지 않고 기존 안내까지 진행한다.
}
