// 원본41a030의 GIF 창을 HTML 부모와 구별해 같은 기본 Gump에 복원한다.
#pragma once
#include "client/GumpCreate.h"
#include <string>
#include <string_view>
namespace netstorm::client { // 원본 BackgroundGif 창의 상태/본문을 모은다.
struct BackgroundGifGumpNode: GumpCreateNode { // 원본0xb0BYTE이며 기본 주소를 그대로 사용한다.
    std::uint32_t event{},first{},second{},bitmap{}; // +a0/+a4/+a8의 종료 입력과 +ac의 소유 원시 GIF 버퍼다.
};
struct BackgroundGifGumpState { // 현재 원문54041e의 signed 종류BYTE다.
    std::int8_t kind{3}; // 보호10.78 초기값이며 호출 시점에 읽는다.
};
struct BackgroundGifCopy { //4a1fe0의 실제8인자에 대응하는 현재 원시 복사 요청이다.
    ScreenPoint position;std::uint32_t bitmap{};std::int32_t stride{};ScreenRect source; // 현재 위치/버퍼/baseWidth와0,0,폭,높이다. 배율 확대와 구별한다.
};
struct BackgroundGifGumpHooks { // 실제 기본/자산/픽셀/입력 경계만 공급한다.
    std::function<BackgroundGifGumpNode*(std::uint32_t)> node;std::function<GumpCreateNode*(std::uint32_t)> base; // 같은 실제 파생/기본 주소다.
    std::function<void(std::uint32_t)> constructBase,destructBase; //4642c0(this,0,0,0,1)/464420의 실제 공통 몸체다.
    std::function<std::string(std::string_view)> pathSpec; //4410f0의 BackgroundGifSpec과 같은 현재 이름이다.
    std::function<bool(std::string_view)> exists; //41ab10의 실제 파일 존재 조회다.
    std::function<std::uint32_t(std::string_view,std::int32_t&,std::int32_t&)> load; //4dae60의 소유 버퍼와 실제 두 출력 DWORD다.
    std::function<void(std::uint32_t)> releaseBitmap; // 원문 free와 같은 소유 반납이다.
    std::function<ScreenPoint(std::uint32_t)> position; //464740의 같은 공통 위치다.
    std::function<void(BackgroundGifCopy)> copy; // 실제 현재 클립/팔레트 인덱스의 원시 복사다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; //419df0의 가상+20이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle; // 나머지 입력은464990에 위임한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> dispatch; //452f90에 현재 세DWORD를 그대로 넣는다.452fe0의 Command 정규화를 추가하지 않는다.
};
class BackgroundGifGump { // 생성/교체/입력/출력/소멸 모두 같은 실제 노드와 버퍼를 사용한다.
public:
    BackgroundGifGump(BackgroundGifGumpState& state,BackgroundGifGumpHooks hooks); // 같은 파생/기본 주소만 미리 검증한다.
    std::uint32_t Construct(std::uint32_t token,std::string_view name) const; //41a030: 기본→종류/bitmap0→현재 GIF→종료 입력0이다.
    void Refresh(std::uint32_t token,std::string_view name) const; //419ec0: 이전 free/0→실제 조회·적재→base/배율 치수다.
    void SetEvent(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; //419e10의 필드 쓰기/flags600이다.
    void Reattach(std::uint32_t token) const; //419df0: 같은 현재 x/y로 parent0/mode0 재부착이다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; //419e40 전체 본문이며 release/세 입력만 판정한다.
    void Draw(std::uint32_t token) const; //419f50: 위치 뒤 현재 base 치수/bitmap을 다시 읽는 원시 복사다.
    void Destruct(std::uint32_t token) const; //419d90: 원문은 free 뒤 bitmap 필드를0으로 쓰지 않는다.
private:
    BackgroundGifGumpNode& Node(std::uint32_t token) const; // 같은 현재 고정 파생 주소만 요구한다.
    BackgroundGifGumpState& state_;BackgroundGifGumpHooks hooks_; // 호출보다 오래 유지되는 실제 상태/장치다.
};
}
