// 같은 Client가 화면 연속 저장의 상태와 실제 프레임 할당을 장기로 소유한다.
#pragma once
#include "client/RawCaptureSequence.h"
#include <filesystem>
#include <map>
#include <memory>
namespace netstorm::o { class RawFileName; } // 실제 캡처는 Client/VFS의 같은 경로 고리를 빌린다.
namespace netstorm::client { //Session/화면 교체는 저장 상태를 복제하지 않는다.
class Client; //현재 Screen/Config/공통 HTML/임시 고리의 실제 소유자다.
std::filesystem::path ResolveCaptureBitmapPath(const std::filesystem::path& root,std::string_view name,o::RawFileName* names=nullptr); //원문 장치 getter 한 칸/상대 root 작성 한 칸을 공유한다. 기본값은 기존 독립 값형 조회다.
std::string CaptureNoticeFontBytes(std::string_view text); //원문 글꼴 바이트를 유지하며 Windows10/11 Unicode 경로의 미지원 문자만[U+코드]로 식별한다. 실제 파일명은 바꾸지 않는다.
class ClientCaptureSequence { //원문 상태/할당 표를 같은 Client 수명에 소유하고 실제 화면/파일 장치에 연결한다.
public:
    explicit ClientCaptureSequence(Client& client); //화면/파일 없이 BSS0와 owner 참조만 준비한다.
    ClientCaptureSequence(const ClientCaptureSequence&)=delete;ClientCaptureSequence& operator=(const ClientCaptureSequence&)=delete; //고정 할당 토큰/현재 장치를 복제하지 않는다.
    void Start(); //4db210만 실행하며 기존 포인터나 할당을 지우지 않는다.
    void Frame(); //436450의 active 관문 뒤4db230이다. 상한이면 표면을 잠그지 않는다.
    void Stop(); //현재 화면/논리 RGB와 실제 BMP·보고/공통 창을 연결한다.
    const CaptureSequenceState& State() const; //같은 고정 상태를 효과 없이 빌린다.
    std::size_t AllocatedFrames() const; //실제 소유 할당 수이며 비영 슬롯 수와 구별한다.
private:
    struct Allocation { std::unique_ptr<std::uint8_t[]> bytes;std::uint32_t size; }; //전체pitch 복사와 이후 현재 폭/높이 출력을 위한 실제 물리다.
    void ReadScreen(); //현재 폭/높이/pitch/색표 토큰만 빌리며 잠금을 새로 얻지 않는다.
    CaptureSequenceHooks Hooks(); //실제 장치를 선택할 때 현재 Client에서 조회한다.
    std::span<std::uint8_t> Memory(std::uint32_t token,std::uint32_t length); //현재 잠금 표면 또는 해당 실제 할당의 범위를 빌린다.
    Client& client_; //이 owner보다 오래 사는 유일한 Client다.
    CaptureSequenceState state_; //005df824/28/2c와1000포인터의 장기 상태다.
    CaptureSequenceScreen screen_; //장치 소비 위치의 현재 Screen 값이다.
    std::span<std::uint8_t> pixels_; //같은 잠금의 전체height*pitch이며 Unlock 전 복사에 쓴다.
    std::map<std::uint32_t,Allocation> allocations_; //Stop에서 반납하고 미완료 출력은 Client 종료 때 RAII로만 반납한다.
    std::uint32_t next_{3}; //1=화면/2=색표와 겹치지 않는 실제 할당 식별자다.
};
}
