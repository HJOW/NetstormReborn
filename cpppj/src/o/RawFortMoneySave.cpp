// 원본 난수의 실제소비와 캡처된stream을 보존하며 Money를 현재SP와 분리한다.
#include "o/RawFortMoneySave.h"
#include <array>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kMoneyRandomLimit=0x3f2U; // 실제004832c1이공유난수에전달하는1010범위다.
constexpr float kMoneyBase=333.0f; // 실제0050d7c4의float32상수0x43a68000이다.
// 도달한경계만검사하며 이전Create/난수소비를 되돌리거나 선행검사로건너뛰지않는다.
template<class Function> const Function& NeedMoney(const Function& function) {
    if (!function) throw std::logic_error("Fort Money 저장 실제 경계 미연결");return function;
}
}
// 실제난수결과0..1009는signed FILD/333가산/float32저장에서정확히표현된다.
void SaveFortMoney(std::uint32_t stream,GameRandom& random,const FortMoneySaveWrite& write) {
    const auto value=static_cast<float>(static_cast<std::int32_t>(random.Next(kMoneyRandomLimit)))+kMoneyBase;
    const auto bits=std::bit_cast<std::uint32_t>(value);std::array<std::uint8_t,4> bytes{}; // 실제FSTP DWORD의비트를일반writer의고정4BYTE원문으로만든다.
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(bits>>(index*8)); // 원본little endian의네바이트만전달한다.
    NeedMoney(write)(stream,bytes);
}
// 단독부모는slot을clamp하지않고isMine/manager진단도추가하지않는다.
void SnapshotFortMoney(std::int32_t slot,GameRandom& random,const FortMoneySaveHooks& hooks) {
    const auto manager=NeedMoney(hooks.manager)(slot);const auto stream=NeedMoney(hooks.create)(manager,"Money");
    SaveFortMoney(stream,random,hooks.write);
}
}
