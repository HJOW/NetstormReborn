// Session없는첫메뉴도같은커서데이터에서원문Cancel과실제Prepare를실행한다.
#pragma once
#include "client/ClientCursorGlobals.h"

namespace netstorm::client { // 상위State전체순서와독립된실제Cursor.cpp leaf연결이다.
class Client;class Renderer;class RawSurfaceStorage; // 실제장치는호출때조회하고이함수보다오래살아야한다.
void CancelCursorForState(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CursorDropState& drop,ClientCursorGlobals& state,CursorLifecycleHooks effects,CursorPlacementHooks placement,std::span<const std::uint16_t> surfaceMap={}); //game.prepare없이00446e70→00445aa0→004458e0을같은전역에서실행한다.
void CancelClientCursorForState(Client& client,RawSurfaceStorage& surface,CursorLifecycleHooks selectedEffects={},std::function<Renderer&()> renderer={}); // 첫메뉴의잠금/빈목록도Project/dirty를생략하지않고선택된다른효과의누락은그자리에서거부한다.
}
