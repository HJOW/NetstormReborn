// 임의시간단축/이미지리샘플링/실패rollback 없이 원문 순서를 수행한다.
#include "client/TitleBitmapLifetime.h"
#include <stdexcept>

namespace netstorm::client { // startup 큰부모와 별도 paint 재출력의 같은 저장소를 사용한다.
namespace { // 선택된 실제 장치만 요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Title bitmap의 선택된 실제 장치 미연결");return function; } //미도달 device를 빈성공으로 채우지 않는다.
void Display(TitleBitmapGlobals g,const TitleBitmapHooks& h,std::int32_t width,std::int32_t height,std::uint32_t index) { //현재표를 Lock 뒤 읽어 raw인자에 전달한다.
    Need(h.lock)();Need(h.copy)({g.width/2-width/2,g.height/2-height/2,g.bitmaps[index],width,{0,0,width,height}});Need(h.unlock)();Need(h.present)();Need(h.flip)();
}
}
bool WaitTitleBitmap(std::int32_t milliseconds,const TitleBitmapHooks& h) { //ms의signed FILD와원문0.001곱을유지한다.
    const double end=double(milliseconds)*0.001+Need(h.wall)();
    for (;;) { if (end<=Need(h.wall)()) return false;Need(h.pump)();if (Need(h.input)()) return true; } //기한도달은pump/input을더호출하지않는다.
}
void RunTitleBitmaps(TitleBitmapGlobals g,const TitleBitmapHooks& h) { //큰부모는 bitmap/paintDepth를 예외에서 되돌리는SEH가없다.
    g.current=-1;++g.paintDepth;Need(h.clip)({0,0,g.width,g.height});Need(h.pump)();
    bool interrupted=Need(h.input)()!=0;std::int32_t width{},height{}; //성공Load가쓰기전에는원문미정local을게임결정으로사용하지않는다.
    if (!interrupted) {
        g.bitmaps[0]=Need(h.load)("title0.gif",width,height);Need(h.palette)("title00.col");Display(g,h,width,height,0);g.current=0;
        g.gameSeconds=Need(h.game)();const double end=Need(h.wall)()+3.0;g.bitmaps[6]=Need(h.load)("title6.gif",width,height);g.bitmaps[7]=Need(h.load)("title7.gif",width,height);
        for (;;) { if (end<=Need(h.wall)()) break;Need(h.pump)();if (Need(h.input)()) { interrupted=true;break; } } //두추가파일을읽은시간도첫3초deadline에포함된다.
        if (!interrupted) {
            Need(h.palette)("title06.col"); //원문palette전용-3항목의뒤0은시간하위를호출하지않고다음항목으로넘긴다.
            if (!interrupted) { if (g.bitmaps[6]) { g.current=6;Display(g,h,width,height,6); }interrupted=WaitTitleBitmap(3000,h); }
            if (!interrupted) { if (g.bitmaps[7]) { g.current=7;Display(g,h,width,height,7); }static_cast<void>(WaitTitleBitmap(3000,h)); }
        }
    }
    std::int32_t screenWidth{},screenHeight{}; //free callback 뒤 매반복의현재SCREEN값을읽고마지막값을fill에쓴다.
    for (std::size_t index=0;index<g.bitmaps.size();++index) { if (g.bitmaps[index]) Need(h.release)(g.bitmaps[index]);screenWidth=g.width;screenHeight=g.height; } //원문은반납한표포인터를0으로재작성하지않는다.
    Need(h.lock)();Need(h.fill)({0,0,screenWidth,screenHeight},0);Need(h.unlock)();Need(h.present)();Need(h.flip)();--g.paintDepth;
}
void RedrawTitleBitmap(TitleBitmapGlobals g,const TitleBitmapHooks& h) { //일반Frame이나BackgroundGif부모로대신하지않는다.
    if (g.current==-1) { const auto width=g.width,height=g.height;Need(h.lock)();Need(h.fill)({0,0,width,height},0);Need(h.unlock)();Need(h.present)();return; }
    Need(h.frontLock)();if (g.current<0 || g.current>=8) throw std::out_of_range("Title 현재bitmap표의 실제 범위");
    if (g.bitmaps[static_cast<std::size_t>(g.current)]) Need(h.frontCopy)({0,0,g.bitmaps[static_cast<std::size_t>(g.current)],g.width,{0,0,g.width,g.height}});Need(h.frontUnlock)();
}
void RequestInitialTitleBitmap(std::span<std::uint32_t,36> next) { next[0]=42;next[35]=0; } //나머지34DWORD는보존한다.
}
