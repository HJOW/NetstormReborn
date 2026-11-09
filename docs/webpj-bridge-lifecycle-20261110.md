# webpj 다리 삭제 전후 훅의 원본 관측 대조와 Native 연결 TYPE 경로(RawBridgeLifecycle 마감) — WB04-E247

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-B의 여섯째 항목이다. [WB04-D2](webpj-squid-destroy-20261105.md)가 옮긴 다리 삭제 훅(`www/core/bridge-lifecycle.js`)은 공통 삭제기 대조(`destroy-x86` Bridge 64행)만 받았고, cpppj 의 전용 시험 `BridgeLifecycleTests.cpp`가 읽는 원본 관측 `bridgeeffects-x86.tsv`는 웹 검사가 읽지 않았다. 또 기준선 뒤 cpppj 변경(`712e7224`, 8줄)이 추가한 Native 몸체 `PreDestroyNative`를 웹이 갖지 않았다. 이 둘을 닫았다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD -- cpppj/src/o/RawBridgeLifecycle.cpp .h`는 `+5/−1`·`+2/−0`이고 커밋은 `712e7224 20261109-34 CP`(Native 표면 수명 가상 삭제와 현재 연결 TYPE 장기 소유권 연결) 하나다. 변경은 `PreDestroy`의 몸체를 `RunPreDestroy`로 빼고, 연결 TYPE을 값(`std::uint32_t linkType`)이 아니라 현재 전역 참조(`const std::uint32_t&`)로 받는 `PreDestroyNative`를 더한 것이다. typed 경로의 결과는 기준선과 같다.

- **② 재반영:** 웹에 `preDestroyNative(bridge, flags, currentLinkType, hooks)`를 추가했다. 연결 TYPE을 값이 아니라 **getter 함수**(`() => 전역 DWORD`)로 받고 각 후보 비교 때마다 다시 읽는다. 기존 `preDestroy`는 같은 몸체(`#runPreDestroy`)를 `() => linkType`으로 호출한다. 후속 `RawSurfaceLifecycle`(3-B 다음 항목, +6)이 이 Native 경로를 소비한다.
- 행 base는 판정한 `712e7224`로 올렸다.

## 2. 옮긴 검사 (`tests/bridge-lifecycle.test.mjs`, 9건)

cpppj `BridgeLifecycleTests.cpp`의 `Scene`·`ReplayLifecycle` 등을 같은 입력·순서로 옮겼다. 원본 fixture는 `cpppj/tests/fixtures/bridgeeffects-x86.tsv`(10.78 `originals` 행만; CD·10.37 행은 범위 밖)를 직접 읽는다.

- **Link 882행:** 링크 객체의 두 참조·state·비대칭 extra 조합에서 `linkNeedsDestroy`(00422290) 판정.
- **Pre 840행:** `preDestroy`(004221b0)의 탐색 범위(`Q:`)·연결 삭제(`D:`)·공통 preDestroy(`A:`) 순서와 풀 전체 바이트(예상한 TYPE/state 외 쓰기 없음). 같은 840행을 `preDestroyNative`로 한 번 더 재생한다.
- **Post 600행:** `postDestroy`(00422300)의 파편 요청(`N:`)·낙하 소리 좌표 비트(`S:`)·좌표를 다시 읽은 탐색(`Q:`)·walker 낙하(`F:`, 동적 TYPE 변경 포함)·공통 postDestroy(`B:`) 순서와 풀 바이트.
- **Delay 242·Pack 242행:** `delayedFallPayload`의 두 `_ftol` 하위 비트 합과 수치 float 비트(비유한·음수·바이트 경계·float 정밀도 경계 포함).
- **계약 3건:** 훅 누락(`next` 없음)·SID 0은 효과 시작 전에 거부(Native 는 getter 가 아닌 값을 거부), Native 가 첫 연결 삭제로 바뀐 전역을 다음 후보에서 읽음(typed 경로는 읽지 못함), 연결 TYPE 비교는 BYTE 슬롯 TYPE 과 전체 DWORD(낮은 8비트만 같으면 삭제하지 않음).
- 합성 객체 번호(다리 50·링크 60·참조 61/62·후속 63·기타 64)는 분석 도구의 입력이며 실제 월드 번호 배치가 아니다.

## 3. 최초 실패와 교정, 대조 강도

최초 실패는 없다. 처음부터 통과했다(fixture 8건). 변형 16개를 하나씩 넣어 확인했다.

- 바로 탐지: 탐색 범위 좌/하단·extra 마스크·dead 비트·walker 마스크·postDestroy 칸 계산에 preDestroy 보정값을 섞는 변형·낙하 포장 시프트/마스크·링크 extra 비대칭·링크 state OR→AND·Native 1회 읽기·공통 emit 인자·walker 비트 비교.
- **살아남은 둘 → 경계 계약 추가:**
  - preDestroy 범위의 보정값 `0x3f7fff58`(거의 1) — 1 ulp 큰 값(`0x3f7fff59`)과 칸이 갈리는 좌표가 있다. `0x3727ffff`(약 1.00135794e-5)는 0 칸, `0x37280000`(정확히 1−보정값)은 1 칸이 되는 입력 둘을 추가해 닫았다.
  - 낙하 소리 효과가 좌표를 바꾸면 postDestroy 는 그 뒤 좌표를 다시 읽어 탐색 칸을 정한다(소리는 바뀌기 전 좌표). 소리 효과가 좌표를 바꾸는 장면을 추가해 닫았다.
- **동등 변형 1:** 낙하 좌표 `_ftol` 상한을 `<`에서 `<=`로 바꿔도 2^63 정확히의 하위 DWORD 가 0 이라 결과가 같다.

## 4. 경계와 다음 시작점

- "다리 가상 삭제 분배"(가상 표 기록값으로 다리 pre/post 를 고르고 연결 삭제를 공통 삭제로 재진입시키는 `Emit`/`EmitBridge`)는 `RawSurfaceLifecycle` 몫이다. 3-B 다음 항목이다. 이 단위의 모듈은 훅 판단과 효과 호출 순서까지다.
- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 실제 연결 탐색기(`RawSquidFinder`) 위의 `makeBridgeLifecycleHooks`는 E43·E88 등 소비자 검사가 쓰고, World 연결은 4-D 다.
- 이 단위로 `RawBridgeLifecycle`은 PORT_MAP `port`(base `712e7224`)다. 3-B의 다음 항목은 `RawSurfaceLifecycle`이다.
