// 실제 Client VFS/설정과 원본 GIF 창을 공통 등록표·픽셀·수명에 연결한다.
#pragma once
#include "client/BackgroundGifGump.h"
#include "client/VFXDraw.h"
#include <memory>
namespace netstorm::o { class BaseFileSystem;class ConfigInterface; } // 실제 파일/설정 소유자를 빌린다.
namespace netstorm::client { // 원문 BackgroundGif 창의 실제 조립 계층이다.
struct RawMenuTree;class GumpPixelTarget; // 같은 공통 장치를 새로 만들지 않는다.
struct RawBackgroundGifContext { // 표시/입력/소멸보다 오래 유지되는 실제 Client 경계다.
    const o::BaseFileSystem* files{};o::ConfigInterface* config{}; // 보호 VFS와 현재 BackgroundGifSpec이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> dispatch; //452f90에 정규화 없이 넣는 원시 큐 사건 세DWORD다.
    std::function<std::uint8_t(std::uint32_t,std::int64_t)> outside; // 보이는 원시 복사가 실제 할당 밖일 때만 필요한 물리 공급이다.
};
class RawBackgroundGifControls { // 같은 실제 노드와 소유 GIF 메모리를 단일 공통 UI에 유지한다.
public:
    RawBackgroundGifControls(RawMenuTree tree,RawBackgroundGifContext context,std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle); // 실제 공통 기본 Help와 같은 입력에 위임한다.
    ~RawBackgroundGifControls(); // 공통 노드의 반납 뒤 소유 이미지를 반납한다.
    std::uint32_t Allocate(); // 원문 new0xb0에 대응하는 실제 고정 파생 저장소다.
    std::uint32_t Create(std::string_view name); // 같은 실제 공장/파일 적재를 수행한다.
    BackgroundGifGumpNode* Node(std::uint32_t token) const; // 현재 실제 기본 주소와 같은 노드만 반환한다.
    const BackgroundGifGump& Backend() const; // 새 Uber도 같은 원문 생성/설정 본체를 빌린다.
    const IndexedImage& Bitmap(std::uint32_t token) const; // 같은 소유 GIF 버퍼의 실제 색 번호를 읽는다.
    std::size_t ImageCount() const; // 실제 소유 메모리 수만 관측한다.
    void Draw(std::uint32_t token,const GumpPixelTarget& pixels) const; // 현재 공통 클립/표면으로 원시 복사를 실행한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; // 같은 실제 입력 본체다.
    void DeleteScalar(std::uint32_t token,std::uint32_t flags); // 원문 이미지 free→기본 소멸→객체 free다.
    void Release(std::uint32_t token); // 공통 scalar의 실제 노드 반납 경계다.
    void Close(); // 미부착/부분 생성도 공통 Kill/Cleanup으로 반납한다.
private:
    struct Impl;std::unique_ptr<Impl> impl_; // 콜백/노드/이미지의 주소를 고정한다.
};
}
