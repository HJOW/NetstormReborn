// 메타 월드의 파생 표시 갱신을 실제 Client/Renderer·같은 현재 Gump 장치에 연결한다.
#pragma once
#include <cstdint> //동일raw객체의BYTE뷰를선언한다.
#include <span> //동기표시중에만현재객체물리를빌린다.
namespace netstorm::client { //Client/controls는동기표시동안유지하며명시World는실제override대여때만필요하다.
class Client;class RawButtonControls;class RawSurfaceWorld; //이 연결은 실제 소유자를 새로 생성하지 않는다.
void RefreshClientMetaSprite(Client& client,RawButtonControls& controls,std::span<const std::uint8_t> raw); //4787c0의기존실제장치본문을World/장기Client표시에서공유한다.
void BindClientMetaSpriteRefresh(Client& client,RawButtonControls& controls,RawSurfaceWorld& world); //현재 메타 World에만 실제88/8c 하위 장치를 설치하며 프레임/통신은 실행하지 않는다.
}
