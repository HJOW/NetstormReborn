// 원본 생성자VT의 객체메뉴 작성/열기 주소를 별개로 조회한다.
#pragma once
#include <cstdint>
namespace netstorm::client {
// 원본 생성자 VT와 서로 다른 두 가상 슬롯을 한 조회 결과로 보관한다.
struct ObjectMenuSlot {
    std::uint32_t vtable,populate,open; //원본VT주소/+a8/+ac이며호스트함수주소로역참조하지않는다.
};
// 지원 목록 밖의 VT는 임의 base 메뉴로 바꾸지 않고 오류로 거부한다.
ObjectMenuSlot ObjectMenuSlots(std::uint32_t vtable);
}
