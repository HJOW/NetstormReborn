// 실제 서버/권한 두 논리 전역만 소유하는 작은 상태이다(cpppj `o/RawAuthorityState.h`). 다른 Squid/Player 상태를 포함하지 않는다.
// 원본 10.78 의 서버 전역(00540bc0)과 권한 전역(00540bc4)의 정상 0/1 소비 경계이며, 원본 DWORD 의 임의 비영 상위 비트까지 보관하는 raw 저장소는 아니다.
// 두 PE 의 초기 DWORD 는 1 이라 둘 다 true 로 시작한다. 같은 Client 의 물리 저장소(SidPool 의 서버 소유자, Process 호스트의 boss/server)가 이 객체를 직접 빌린다.
// 사용: const authority = new RawAuthorityState(); authority.server = false; state.bindAuthority(authority).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-D5c: Process 호스트 상태가 빌리는 권한 소유자로 추가)
export class RawAuthorityState {
    // 서버 관문이 읽는 현재 값이다(00540bc0). 초기값 true.
    server = true;

    // 권한 관문이 읽는 현재 값이다(00540bc4). 초기값 true.
    boss = true;
}
