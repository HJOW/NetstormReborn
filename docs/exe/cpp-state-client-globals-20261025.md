# State 정리를 위한 실제 Client 전역 공유

2026-10-25 단위429. 기준은10.78이며 현재 PC와 마지막 전체 정밀 디컴파일 PC는 모두 **HJOW-Athlon**이다. 마지막 전체 정밀본 생성일은2026-10-23이고 이번에는 새 디컴파일이나 원본 게임을 실행하지 않았다. 이전 통합 제품은33a98ea8의2870검사·실제 메뉴·자연 Early2 검증이다.

## 실제 소비자와 같은 주소

Edit State1의 정리 전에 쓰던 값을 Session 소멸로 잃지 않도록 기존 `ClientLocalStateGlobals`에 다음 값을 소유시켰다. 별도 주소 사전이나 세션 반납 뒤 복사하는 동기화는 사용하지 않는다.

| Native 주소 | 실제 소유 필드/API | 연결한 기존 소비자 |
|---|---|---|
| 005c85c0 DWORD | `ZonePending()` | Fort 요청 작성, `RawLocalMissionState` |
| 0055a420 DWORD | `SourceType()` | State, 실제0x28 Reply의 `sourceTicks` |
| 005c85d4 정상0/1 | `SavedGame()` | State, 로그인/타이머/Reply, 내용물 통지, 섬 H칸 전파 관문과 Fort Money wrapper |
| 005c7a74 DWORD | `Begin()` | 실제 Login, Ready, 로그인 타이머 |
| 005c85a0 DWORD | `CentralCreated()` | 실제 metadata 송신 부모 |
| 005c8618 double | `CaptureEnd()` | Zone 설치, LoginBegin/Reply |
| 005c85f8 double | `ConnectEnd()` | State8/9 및 Connected |
| 005c85f0 double | `LoginEnd()` | 로그인 타이머, Dais, 충돌/Flare 효과 |

실제 Client 서비스는 기존 `localStateGlobals`를 전달한다. 외부 소유자가 없는 독립 Session만 최초 BSS0 owner를 한 번 만들며, Login의 다섯 블록과 기존00544d58 활동 기한도 이 owner를 직접 빌린다. 일반 Session 생성자/소멸자는 이 새 전역을 초기화하지 않는다. 이것이 Native cleanup을 실행한 결과라는 뜻은 아니다.

섬 H칸의 받침/종유석 owner 전파도 실제 표면 모듈 생성 전에 같은 SavedGame 주소를 전달한다. 독립 `IslandPostPopState`의 기존2필드 aggregate는 유지한다. Fort Money는 기존 wrapper 진입 직전에 현재 SavedGame을 값형 `economyBlocked`에 캡처하는 방식이므로 이 호출 내부까지 완전한 직접 별칭으로 바꾼 것은 아니다.

`GunImpactState`는 기존3필드 aggregate를 보존하고 마지막 선택형 `double*`를 추가했다. `SuppressedTime()`은 매번 같은 double의 참조를 반환한다. Session은 `impactState` 생성 시 이미 같은 LoginEnd 주소를 전달하므로 이후 Flare가 캡처하는 `const double&`도 같은 주소다. 기존 독립 호출은 선택형 포인터가 없을 때 원래 값형 필드를 사용한다. 충돌 효과의 실제 관문과 Session 폭발 전처리도 이 현재값을 읽는다.

`UberGump::InitializeMission`의 Fort 요청 작성은 실제 `State().next`144BYTE와 `ZonePending()`에 직접 쓴다. `MissionFortLoadRequest`의 next/sequence는 작성 뒤 결과 캡처다. 기존 Post→다음 루프 Session 설치 순서를 유지하고, 아직 복원하지 않은 전체 State7을 먼저 실행하지 않는다.

## 같은 장치의 좁은 초기화 API

`Client::ResetGameClock()`은 기존 `clock_.Reset()`만 호출한다. 원문00460e70처럼 누적 정지 시간만 지우며 원점·중첩 정지 깊이·저장 게임 시각·현재 FrameTime을 다시 만들거나 캡처하지 않는다.

`ClientAudio::WriteSongEndBits(uint64_t)`는 같은 `scene_.songEnd`의 객체 표현8BYTE만 memcpy한다. 원문 cleanup의00565e68 쓰기용 API이며 곡 요청·장면 시작·시각 조회·난수 소비·장치 재설정은 수행하지 않는다. 이 API들을 전체 Native cleanup의 effect adapter에 연결하는 작업은 후속이다.

## 검증과 남은 경계

동결23소스의 **첫 빌드가 종료0·컴파일 경고/오류0**으로 완료됐고, 같은 네 실행 파일에서 **집중63개·실패0**을 확인했다. 신규 검사는10개이며 기존 실제 Editor checkpoint23/후처리4·Core 갱신3·LocalLogin1·Native 요청2 및 각 원문 관측을 함께 확인했다. 새 Session 검사4개는 두 Session의 전후 보존, 실제 handshake/metadata 분기·0x28 비동기 재수신, 실제 내용물 음향 관문, 요청 writer의 현재값 재조회/unsigned 감김을 통과했다.

[실행 보고서](../../cpppj/state-client-globals-runtime-20261025.json)는 필터별 실제 관측 수/종료 코드·로그 SHA와23소스/4실행 파일을 기록한다. 게임 실행 파일 SHA는 **16203b9a…**다. 원본/CD·AGENTS·기존 정밀본/고정 관측을 포함하는 보호1821파일도 같은 기준과 일치했다. 단위429에서는 전체 CTest나 긴 GUI 플레이를 반복하지 않고 다음 기능 묶음의 같은 최종 제품에서 통합한다. 이전2870/GUI/자연 Early2 결과를 이번 제품의 통합 결과로 인용하지 않는다.

`SavedGame()`은 기존 bool 소비자의 정상0/1 표현이다. 임의 Native DWORD 비트나 상위 BYTE 보존까지 복원했다고 주장하지 않는다. 게임 시각 tick/난수 seed, Zone6BYTE/수신22DWORD, 일부 기한, 서버/boss와 revision, Graph/섬/Chunk/장부의 Session 지역 소유도 아직 남는다. 전체 cleanup/prefix와 실제 Edit 메뉴 State1 설치, 사용자 편집/저장/테스트 복귀는 계속 미완료다. 모든 미션 자연 완주·진행 저장/재실행·원본과 완전 동일성도 이번 단위의 검증 범위를 넘는다.
