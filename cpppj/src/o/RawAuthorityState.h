// 실제 서버/권한 두 논리 전역만 소유하며 다른 Squid/Player 상태를 포함하지 않는다.
#pragma once

// 같은 Client의 물리 저장소와 현재 전역이 직접 참조하는 작은 권한 소유자다.
namespace netstorm::o {
// PATCH00540bc0/00540bc4와 CD00540a28/00540a2c의 정상0/1 소비 경계다.
// 원본 DWORD의 임의 비영 상위 비트까지 보관하는 raw 저장소는 아니다.
struct RawAuthorityState {
    bool server{true}; // 두 PE의 초기 DWORD1을 논리true로 시작하며 서버 관문에서 현재값을 읽는다.
    bool boss{true}; // 두 PE의 초기 DWORD1을 논리true로 시작하며 권한 관문에서 현재값을 읽는다.
};
}
