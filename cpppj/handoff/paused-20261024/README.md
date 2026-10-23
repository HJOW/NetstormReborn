# 목표 일시중지 시점의 미적용 사본

사용자가 현재 단위387 완료 후 목표 일시중지를 요청하여, 이미 준비된 후속 자료만 동결 보관한다. 이 폴더의 네 patch는 공용 제품에 적용하지 않았다. 새 기능 단위388의 구현·등록·실행도 시작하지 않는다. 현재 단위의 검증과 결과는 [387 기록](../../../docs/exe/cpp-session-transition-runtime-20261024.md), 전체 재개 위치는 [LEFT_JOBS.md](../../../LEFT_JOBS.md)에 있다.

`manifest.json`은 원시 patch 네 개와 기존 검증 자료 일곱 개의 출처·SHA256·복사 바이트 동일성을 기록한다. proof의 사본 검사 성공과 현재 공용 제품의 통합 성공은 별개다. 원시 자료에는 저장소 하위 cwd에서 `git apply`가 skipped하고 종료0이었던 과거 오판도 있으므로, 재개 시 실제 파일 생성·내용·SHA를 함께 확인해야 한다.

재개할 때 다음 의존 관계를 먼저 검토한다.

1. `core-refresh-target.patch`는 정상 타입을 TYPE165/VT507950+80→4478f0/`RawTempleRitual::Count`로 해석하는 교정이다. 앞선 Player152/`RawSquidLevel::Get` 해석은 폐기한다. 원문 연산·최초 fixture는 유지한다.
2. `core-refresh-session.patch`는 같은 Login/Core/Pool과 TYPE 기술 비트를 연결하는 제안이다. 새 Session 검사3개는 아직 실행하지 않았다. ExistingEditor의 CoreWrite는 editor/post에서 자연 반환하므로 해당 경로에 refresh를 임의 호출하지 않는다. [사본 범위](../../../docs/exe/cpp-player-core-refresh-session-next-20261024.md)를 확인한다.
3. `client-subscriber.patch`는 실제 My/Login 두 DWORD를 Client에서 공유하는 제안이다. 순수 소유자 검사6개·구문 검사와 실제 Client 통합 검사3개의 미실행을 구별한다. 일반 미션의 기존 hard1 쓰기 시점은 원문 동등성이 미확정이다. [Subscriber 동결 기록](../../../docs/exe/cpp-client-subscriber-globals-proposed-20261024.md)을 확인한다.
4. `client-colorsort-primary.patch`는 실제0x3800 소유자를 Client 소비자에 연결하는 제안이다. 독립 Native80/격리3검사 결과와 새 통합 검사 미작성·컴파일0·실행0을 구별한다. [Client 동결 기록](../../../docs/exe/cpp-client-colorsort-primary-proposed-pause-20261024.md)을 확인한다. 함께 보관한 `RawColorSortPrimary` 소스·검사는 공용 CMake에 등록하지 않았다.

현재387의 TYPE 기술 비트는 이미 공용에 적용돼 있다. 다른 사본의 기준과 겹치는 파일은 재개 시 문맥을 다시 대조해야 한다. 네 patch를 그대로 연속 적용하면 된다고 보장하지 않는다. 실제 Client Corealias 공유는 Player 해제 전 원문 정리와 단일 소유자 수명을 갖추기 전에 앞당기지 않는다.

[State/World 정리 재사용 표](../../../docs/exe/cpp-state-world-cleanup-reuse-20261024.md)는 기존34개 몸체와 추가12개 hook을 정리한 동결 문서다. 새 정리 부모 호출이나 제품 배선을 구현했다는 뜻은 아니다. Subscriber v1의 관측0 보고서는 실패 자료로 보존하며, v2의120 checkpoint 자료와 합산하지 않는다.

후속 자료의 저장 커밋은 작업 손실을 막기 위한 인수인계다. 전체 Early/캠페인/Demo 사용자 완주·모든 메뉴와 Edit·화면/음향/규칙의 완전 동일성은 미완료 상태로 남는다. 사용자가 목표를 재개하기 전에는 후속 적용을 진행하지 않는다.
