# 데모와 후속 캠페인의 대응 관계

2026-10-16 사용자 제보를 원본10.78의 메뉴·미션 자원과 대조했다. 현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-16**이다. 읽기 전용 아카이브 조사이며 새 원본 게임 실행은 없다. CD10.72는 별도 확인이 필요하다.

## 확인된 대응

| 데모 순서 | 데모 표시 제목 | 미션 키 | 대응 캠페인 | 원본 메뉴 근거 |
| --- | --- | --- | --- | --- |
| 1 | The Storm Rages | DemoStormRages | 이번 조사에서 대응 캠페인 미확정 | `tell.english` `[DEMOVILLE]` |
| 2 | Demo of "Guard My Back" | GuardMyBack | **캠페인2-4 Guard My Back** | `offical4.english` 13행의 네 번째 `MissionBegin,GuardMyBack` |
| 3 | Demo of "Dissolved Alliance" | DissolvedAlliance | **캠페인1-6 Dissolved Alliance** | `offical3.english` 22행의 여섯 번째 `MissionBegin,DissolvedAlliance` |

`tell.english` 462~463행의 데모2·3도 각각 `StartDemo,GuardMyBack`·`StartDemo,DissolvedAlliance`를 호출한다. **캠페인과 데모가 같은 미션 키와 같은 `.english`/`.fort` 자원을 사용한다.** 별도의 데모용 맵 복사본을 만들어 복원할 근거는 없다. 사용자의 “우리 쪽에도 AI가 붙는 변형판”이라는 설명은 아래 원본 시작 분기와 일치한다.

`offical3`은 Struggle For Freedom, `offical4`는 A Nation Rises의 원본 파일 이름이다. `offical`이라는 원본 철자도 유지한다. 캠페인1-6의 기존 화면 근거는 [시작 캡처 대조](../screens/dissolved-alliance-start.md)를 참고한다.

## 데모 실행에서 달라지는 부분

- 원본 `StartDemo`004d7ffb는 같은 미션 생성00463b70 뒤00463a00(1)로 현재 미션+68의 데모 플래그를 켠다. 일반 캠페인은 같은 자원을 `MissionBegin`으로 시작한다.
- 복원한 공통 Start는 일반 미션에서 로컬 소유자의 AI 생성을 생략하고, 데모에서는 실제 사제가 있는 **로컬 소유자도 AI 생성에 포함**한다. 일반 캠페인에 데모 로컬AI를 잘못 활성화하면 안 된다.
- 두 스크립트에 모두 `ai1Name="Player"`, `ai1TimeBetweenMoves=3`, `ai1Tech`가 있다. `GuardMyBack`에는 원문 주석 “This if for the demo ONLY.”와 `ai1AllyList="2"`가 있다. 일반 캠페인의 사람 편 동맹도 로컬AI 분기와 별도로 정확히 초기화해야 한다.
- 두 스크립트의 `demoRestartTimer=360`은 데모 모드의 종료/재시작 게이트가 소비한다. 같은 맵에 AI를 붙이는 것 외에 재생 수명도 복원해야 한다.
- 현재 데모 여부는 승패 사건 섹션 이름의 `Demo` 접두 선택에도 쓰인다. `GuardMyBack`의 `[Demo]` 안내는15초 Timeout을 갖는다. 일반 캠페인의 성공/실패·완료값·Next Mission을 데모 종료와 혼동하지 않는다.

이는 자원과 복원한 원본 시작/프레임/승패 몸체의 정적 근거다. 모든 실행 차이가 이 목록뿐이라는 결론이나 두 모드의 자연 플레이 동일성을 검증한 결과는 아니다. 관련 코드는 `RawMissionStart.cpp`, `RawMissionFrameGate.cpp`, `RawMissionOutcome.cpp`이며 [데모 초기화 복원 기록](../exe/cpp-demo-initialization-20261016.md)과 함께 읽는다.

## 캠페인1-6·2-4 작업 시 재사용할 근거

1. 데모에서 복원한 동일 Fort 로더·Graph·건물/프로세스·전투/AI를 같은 미션의 캠페인 경로에서도 사용한다. 모드별로 별도 게임 규칙을 복사하지 않는다.
2. 같은 초기 자원에서 데모는 로컬AI 있음, 캠페인은 로컬AI 없음으로 시작하는지 비교한다. 기술·SP·방향별 동맹·AI 색·참여 사제 상태를 각각 확인한다. `myTech`와 `ai1Tech`의 실제 목록/순서도 별도로 읽는다.
3. 데모의360초 수명과 자동 안내, 캠페인의 사람 입력·성공/실패·다음 미션·진행 저장을 각각 확인한다. 캠페인1-6의 Next Mission은 `TheNoose`, 캠페인2-4는 `Surrounded`다.
4. 데모의 헤드리스 Load/Start 통과를 캠페인 완주·GUI 재현의 완료로 간주하지 않는다. 실행시각·공통 난수·전략 프레임 및 화면/음향 대조가 더 필요하다.

현재 목표의 캠페인 범위는1-1~1-2다. 이 문서는 이후1-6·2-4 작업을 위한 인수인계다.

## 원본 자원 식별

아래 값은 `originals/netstorm.tarc`를 `tools/taff.py`로 읽어 복호화한 **엔트리 본문 전체**의 SHA-256이다. 경로는 아카이브 내부 경로이며 보호 파일은 변경하지 않았다.

| 엔트리 | 바이트 | SHA-256 |
| --- | ---: | --- |
| `\d\guardmyback.fort` | 3256 | `fd3c1585985b6097d2dc00ea1a560dfc12876f7c66476888b69d25cd0113a777` |
| `\d\guardmyback.english` | 3630 | `5296e6b52312585f3beca8341b37fe26aedad60b19fca5dc21f6b464d452163e` |
| `\d\dissolvedalliance.fort` | 3448 | `c029ba299435686ba6f04b3cf0da579f6b53abdd4115d8efe9ed4944f191da1d` |
| `\d\dissolvedalliance.english` | 2099 | `fcc7fbbef99515c4ad660a11da18318e48bd63b5467b6aeedb6c51a16d1a06e5` |
