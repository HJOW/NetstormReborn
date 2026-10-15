// Player 내용물 통지의 SP 접두를 같은 실제 저장소/추적 offset의 현재 조회에 연결한다.
#pragma once
#include "o/RawPlayerContentNotice.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 네 읽기는 값을 보관하지 않고 호출할 때마다 같은 저장소의 원본 필드를 조회한다.
struct SpStoreNoticeBindings {
    PlayerContentNoticeRead<std::uint32_t> cursorA; // 00568af8: 실제 Get/Set/Rotate가 쓰는 offset이다.
    PlayerContentNoticeRead<std::uint32_t> cursorB; // 0054d4e0: 외부 사제 상태가 소유한 같은 추적 offset이다.
    PlayerContentNoticeRead<std::uint32_t> firstSpPool; // 실제 첫 배열 주소가 NULL인지 읽으며 포인터 토큰을 발명하지 않는다.
    PlayerContentNoticeRead<std::int16_t> firstPermutation; // 실제 Get/Set/Rotate가 공유하는 순열 배열의 첫 WORD다.
};
// store와 BindTrackingOffset에 연결한 외부 상태는 반환 조회기/통지 모듈보다 오래 살아야 한다.
// 첫 초기화 전에 추적 offset을 연결하지 않았으면 사본으로 대체하지 않고 거부한다.
SpStoreNoticeBindings BindSpStoreNotice(const ScrambledSpStore& store);
}
