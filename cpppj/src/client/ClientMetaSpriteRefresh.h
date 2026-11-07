// 메타 월드의 파생 표시 갱신을 실제 Client/Renderer·같은 현재 Gump 장치에 연결한다.
#pragma once
namespace netstorm::client { // 세 소유자는 바인딩한 현재 월드의 수명 동안 유지돼야 한다.
class Client;class RawButtonControls;class RawSurfaceWorld; //이 연결은 실제 소유자를 새로 생성하지 않는다.
void BindClientMetaSpriteRefresh(Client& client,RawButtonControls& controls,RawSurfaceWorld& world); //현재 메타 World에만 실제88/8c 하위 장치를 설치하며 프레임/통신은 실행하지 않는다.
}
