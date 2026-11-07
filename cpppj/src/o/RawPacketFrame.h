// 단일/집합 프레임을 Player/22칸 Registry 없이 같은 실제 패킷 표로 분배한다.
#pragma once
#include "o/RawPacketDispatch.h"
namespace netstorm::o { // Client와 R의 일치하는137종/길이표에 사용하는 공통 부모다.
bool DispatchPacketFrame(RawPacketDispatch& dispatch,PacketConnection& connection,std::span<const std::uint8_t> frame,const PacketDispatchHooks& hooks); // frame은 실제 mutable 전송 버퍼여야 하며 observer/receiver 수명은 반환까지 유지한다.
}
