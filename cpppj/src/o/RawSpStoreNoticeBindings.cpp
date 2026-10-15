// signed 추적 DWORD와 실제 배열 주소를 원본 비교 폭/조건에 맞춰 현재 읽는다.
#include "o/RawSpStoreNoticeBindings.h"
#include <bit>

namespace netstorm::o {
// 모든 조회기는 같은 객체를 참조하며 초기화/회전 뒤 별도 동기화를 요구하지 않는다.
SpStoreNoticeBindings BindSpStoreNotice(const ScrambledSpStore& store) {
    static_cast<void>(store.TrackingOffsetCurrent()); // 미연결 추적 전역은 첫 바인딩에서 거부한다.
    return {
        PlayerContentNoticeRead<std::uint32_t>::Bind([&store] { return store.OffsetCurrent(); }),
        PlayerContentNoticeRead<std::uint32_t>::Bind([&store] { return std::bit_cast<std::uint32_t>(store.TrackingOffsetCurrent()); }),
        PlayerContentNoticeRead<std::uint32_t>::Bind([&store] { return store.FirstPoolCurrent()!=nullptr ? 1U : 0U; }),
        PlayerContentNoticeRead<std::int16_t>::Bind([&store] { return store.Permutation().front(); })
    };
}
}
