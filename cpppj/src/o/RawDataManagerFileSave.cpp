// 현재 파일 이름과 이미지 포인터/길이의 서로 다른 읽기 시점을 원본 저장 부모와 맞춘다.
#include "o/RawDataManagerFileSave.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 비정렬 원본 DWORD를 실제 네 BYTE에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(index*8); //같은 네 BYTE만 합친다.
    return value;
}
// 선택된 원본 하위가 없는 경우 성공으로 대신하지 않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("DataManager 실제 파일 저장의 하위 장치 미연결");return function;
}
}
// 저장 억제 여부와 무관하게 이미지를 먼저 생성하며 현재 포인터의 물리 계약만 호스트에서 검사한다.
void SaveDataManagerFile(std::span<std::uint8_t> manager,const DataManagerFileSaveState& state,const DataManagerFileSaveHooks& hooks) {
    if (manager.size()<12) throw std::invalid_argument("DataManager 실제 파일 저장의 manager 누락");
    std::array<std::uint8_t,20> image{};std::fill(image.begin()+16,image.end(),std::uint8_t{0xff}); //원본 첫 네 DWORD0/mark-1이다.
    std::uint32_t pointer{};bool captured{},released{};
    const auto release=[&] { //직렬화 완료 뒤에는 보관한 포인터를, 생성 중 호스트 예외에서는 현재 포인터를 한 번만 반납한다.
        if (released) return;released=true;const auto current=captured ? pointer : Read(image,0);if (current) Need(hooks.release)(current);
    };
    try {
        Need(hooks.image)(manager,image);pointer=Read(image,0);captured=true;
        if (!state.readOnlyForts) {
            if (state.localOwner>=state.hiddenPlayers.size()) throw std::out_of_range("DataManager 저장의 현재 숨김 표 물리 인덱스 범위");
            if (!state.hiddenPlayers[state.localOwner]) {
                Need(hooks.permissions)(Need(hooks.fileName)(manager),0x180U);
                const auto length=Read(image,8);const auto name=Need(hooks.fileName)(manager); //원본은 둘째 가상 이름 조회 전에 길이 인자를 보관한다.
                Need(hooks.write)(name,pointer,length);
            }
        }
        release();
    } catch (...) { release();throw; }
}
}
