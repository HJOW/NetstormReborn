// PATCH004be960의267BYTE SP writer를같은MemStream16BYTE/공유게임난수에실행한다.
#pragma once
#include "o/RawMemStream.h"
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 원문의512회Next(32)와인라인stream쓰기장치를빌리며난수/버퍼를별도로소유하지않는다.
struct FortSpWriteHooks {
    std::function<std::uint32_t(std::uint32_t)> nextRandom; //같은GameRandom.Next를그시점에호출한다.정상호출은항상limit32다.
    MemStreamHooks stream; //buffer와WriteCapacity진단만사용한다.allocate/release나일반WriteMemStream는호출하지않는다.
};
// float32 caller→실제_ftol signed64절삭의하위DWORD→각bit의16잡음→8BYTE/8WORD/16DWORD총88BYTE다.
// 비유한/64비트범위밖의masked_ftol불확정값은하위DWORD0이다.진단은memstream.h36이며복귀뒤현재필드를읽는다.
// stream과buffer가같은실제할당을가리켜도사본으로분리하지않는다.buffer조회는순수물리뷰여야한다.
void WriteFortSp(std::span<std::uint8_t> stream,float value,const FortSpWriteHooks& hooks);
}
