// 메타전투의물리효과를같은실제Client/Session/Pool에바인딩한다.
#pragma once
namespace netstorm::client { //상태성공/가짜객체를만들지않고실제사용시현재owner를조회한다.
class Client;struct NativeMetaBattleCreateHooks; //같은Client와원문생성부모의장치묶음이다.
void BindClientMetaBattleObjects(NativeMetaBattleCreateHooks& hooks,Client& client); //현재TYPE·실제공장/QA/물리/가상Pop/진단을연결한다.Rig/23h등록은caller가완성하며기존animate는보존한다.
}
