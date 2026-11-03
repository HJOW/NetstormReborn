// startup 큰부모의현재8칸/표시/반납을실제Client장치에연결한다.
#include "client/ClientTitleBitmaps.h"
#include "client/ClientMain.h"
#include "client/GifFileRead.h"
#include "client/ClientStaticStrings.h"
#include "o/RawFileName.h"
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace netstorm::client { // Native8칸을일반BackgroundGif캐시에합치지않는다.
ClientTitleBitmaps::ClientTitleBitmaps(Client& client):client_(client) {} //BSS0과같은참조만구성한다.
TitleBitmapGlobals ClientTitleBitmaps::Globals() { return {bitmaps_,current_,paintDepth_,client_.screenWidth_,client_.screenHeight_,client_.time_->game}; } //FrameTime.game에직접쓰며별도시계를만들지않는다.
bool ClientTitleBitmaps::Active() const { return paintDepth_!=0; } //정상반납은큰부모의마지막--만수행한다.
std::string FormatTitleBitmapPath(o::RawFileName& names,std::string_view literal) { const std::array<o::FileNamePart,3> parts{{{-2,""},{-2,"d"},{-3,literal}}};return std::string(names.Format(parts)); } // 원문540cec의 빈 문자열과540cf8의 고정 d를 사용하며 설치 경로는 현재 VFS가 담당한다.
std::string ClientTitleBitmaps::Path(std::string_view literal) { return FormatTitleBitmapPath(client_.staticStrings_->FileNames(),literal); } // 실제 그림·팔레트 적재가 같은 경로 작성기와 고리를 사용한다.
std::uint32_t ClientTitleBitmaps::Load(std::string_view literal,std::int32_t& width,std::int32_t& height) { auto image=ReadBackgroundGifFileImage(client_.files_,Path(literal),width,height);if (next_==0x73ffffffU) throw std::overflow_error("Title bitmap 실제주소별칭소진");const auto token=next_++;images_.emplace(token,std::move(image));return token; } //성공반환뒤에만원문표slot을쓴다.
void ClientTitleBitmaps::Copy(const TitleBitmapCopy& copy,bool front) { auto& screen=*client_.screen_;const auto& image=images_.at(copy.bitmap); //원문stride/영역은그대로유지하고할당밖원문힙을임의padding으로채우지않는다.
    const auto frameBytes=std::uint64_t(screen.Width())*std::uint64_t(screen.Height()); // 현재 DIB의 실제 화면 영역이며 그림 할당을 늘리는 크기가 아니다.
    if (front && copy.x==0 && copy.y==0 && copy.stride==screen.Width() &&
        copy.source.left==0 && copy.source.top==0 && copy.source.right==screen.Width() && copy.source.bottom==screen.Height() &&
        frameBytes>image.indices.size()) {
        // 원본은 작은 그림 밖 힙을 읽지만 현대 Windows 콜백에서는 예외로 종료된다. 같은 DIB의 완성된 화면을 다시 출력해 이미 그린 타이틀과 여백을 보존한다.
        screen.Update({0,0,screen.Width(),screen.Height()});return;
    }
    if (front) screen.Lock();
    try { screen.BlitBuffer(copy.x,copy.y,image.indices,copy.stride,copy.source); }
    catch (...) { if (front) screen.Unlock();throw; } //현대DIB잠금만반납하며원문전역/이미지/치수는rollback하지않는다.
    if (front) { screen.Unlock();screen.Update({0,0,screen.Width(),screen.Height()}); } //원문windowedDIB→창복사를현행호환표면으로수행한다.독점DirectDrawfront표면을새로합성하지않는다.
}
TitleBitmapHooks ClientTitleBitmaps::Devices() { TitleBitmapHooks h;auto& screen=*client_.screen_;
    h.pump=[this] { if (!client_.PumpMessages()) throw TitleBitmapQuit(); };h.input=[this] { return client_.input_.Pop(true).code; }; //실제WM_QUIT은게임오류대화창대신Run정상반환이며입력큐의release처리는별도다.
    h.game=[this] { return client_.clock_.GameSeconds(timeGetTime()); };h.wall=[this] { return client_.WallSeconds(); }; //원문pause가능시각과벽시각을구별한다.
    h.clip=[&screen](auto rect) { screen.SetClip(rect.left,rect.top,rect.right,rect.bottom); };h.lock=[&screen] { screen.Lock(); };h.unlock=[&screen] { screen.Unlock(); };
    h.present=[&screen] { screen.PresentPaletteClear(); };h.flip=[&screen] { screen.Flip(); }; //현재같은전체viewport/bit4관문을사용한다.
    h.load=[this](auto literal,auto& width,auto& height) { return Load(literal,width,height); };h.palette=[this](auto literal) { client_.LoadPaletteFile(Path(literal),1); }; //fortPal복구는State42바깥이담당한다.
    h.copy=[this](const auto& copy) { Copy(copy,false); };h.fill=[&screen](auto rect,auto color) { screen.FillRect(rect,color); };h.release=[this](auto token) { if (images_.erase(token)!=1) throw std::out_of_range("Title bitmap 같은할당반납누락"); }; //표의staleDWORD와살아있는할당을구별한다.
    h.frontLock=[&screen] { if ((screen.Flags()&3U)==3U) screen.Lock(); };h.frontUnlock=[&screen] { if ((screen.Flags()&3U)==3U) screen.Unlock(); };h.frontCopy=[this](const auto& copy) { Copy(copy,true); }; //4a17c0/17e0은bit1/2관문이며windowedDIBfrontcopy는자체잠금이다.
    return h;
}
void ClientTitleBitmaps::Run() { RunTitleBitmaps(Globals(),Devices()); } //원문예외시자동정상cleanup/paintDepth감소를합성하지않는다.
void ClientTitleBitmaps::Redraw() { RedrawTitleBitmap(Globals(),Devices()); } //같은표/번호/Screen을현재WM_PAINT에서빌린다.
void Client::RunInitialTitleBitmaps() { if (!titleBitmaps_) titleBitmaps_=std::make_unique<ClientTitleBitmaps>(*this);titleBitmaps_->Run(); } //실제State42에서만owner를lazy생성하며기본UI에추가명령을노출하지않는다.
}
