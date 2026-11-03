// 같은Client가시작title8칸/WM_PAINT깊이와실제픽셀할당을소유한다.
#pragma once
#include "client/TitleBitmapLifetime.h"
#include "client/VFXDraw.h"
#include <map>
#include <string>
#include "o/RawFileName.h"

namespace netstorm::client { // 호스트/CD의Uber title_ 캐시와별도인원문startup owner다.
// 설치 디렉터리는 VFS에 맡기고 현재 파일명 고리에서 원본의 상대 경로를 작성한다.
std::string FormatTitleBitmapPath(o::RawFileName& names,std::string_view literal);
class Client; // 같은Client의현재Screen/File/시계/입력을빌린다.
// 시작 그림의 실제 할당과 재출력 상태를 같은 Client 수명 동안 보관한다.
class ClientTitleBitmaps {
public:
    explicit ClientTitleBitmaps(Client& client); // 생성은BSS0만구성하며I/O/표시/대기를하지않는다.
    void Run(); //4ab3e0의같은저장소와실제장치를연결한다.
    void Redraw(); //WM_PAINT의4ab2d0을같은현재bitmap으로수행한다.
    bool Active() const; //현재depth의비영만읽는다.
private:
    TitleBitmapGlobals Globals(); //현재Client의SCREEN과FrameTime.game을직접빌린다.
    TitleBitmapHooks Devices(); //동기callback은owner보다오래보관하지않는다.
    std::string Path(std::string_view literal); // 같은 고리의 빈 경로·고정 d·파일명으로 설치 경로 중복을 피한다.
    std::uint32_t Load(std::string_view literal,std::int32_t& width,std::int32_t& height); //781의필수Open/Decode-before-close를현재실제할당에연결한다.
    void Copy(const TitleBitmapCopy& copy,bool front); //그림고유폭으로교정하지않고현재raw복사인자를검사한다.
    Client& client_; //현재장치와같은FrameTime/입력을빌리는유일한실제Client다.
    std::array<std::uint32_t,8> bitmaps_{}; //005c7c24..40의최초BSS0이다.
    std::int32_t current_{};std::uint32_t paintDepth_{}; //005c7c20/0054dc54의최초BSS0이다.
    std::map<std::uint32_t,IndexedImage> images_;std::uint32_t next_{0x73000000U}; //host픽셀소유와원문DWORD별칭을유지하며free뒤표DWORD는보존한다.
};
}
