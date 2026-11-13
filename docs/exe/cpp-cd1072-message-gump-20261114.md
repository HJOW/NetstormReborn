# CD 10.72 메시지 Gump 판본 복원 (제품 1307, 2026-11-14)

마지막 디컴파일 PC: **HJOW-Athlon**. `extracted/refined/originalCD/NETSTORM.c`의 기존 디컴파일을 사용했으며 원문 변경이나 재디컴파일은 없다. 보호 CD 실행 파일은 읽기만 했다.

CD 자료로 클론을 시작하면 `WarningMessages::Impl`이 첫 실제 메시지 표시보다 먼저 `MessageGump`를 생성한다. 기존 생성자는 공유 Gump 판본이 10.78이 아니라는 이유로 `Pause 메시지 실제 TextGump/이름/번역/측정 연결 누락`을 던졌다. CD 생성자를 연결해 이 시작 차단을 제거하고 CD의 문구와 메시지 필드를 복원했다.

## 원문과 구현

CD `004c8b40`은 20칸 MessageBoard 검색과 파생 생성이 합쳐진 함수다. 그중 메시지 생성 부분을 기존 `MessageGump::ConstructCd`로 연결했다. 원문의 `004f1650(0xc0)` → `004b7280`(기본 TextGump) → 파생 필드 → 이름 조회 → 종류별 효과 → `004b75c0(0,0)`(Fit) 순서를 따른다. 기존 `TextGump`는 이미 CD 기본 가상표 `00505dc0`와 같은 head·글자 문맥을 지원한다.

| 필드 | CD 10.72 근거 | 구현 |
| --- | --- | --- |
| 파생 가상표 | `004c8b40`: `00506658` | CD 생성 뒤 `MessageGumpNode::vtable` |
| 글자 체인 | TextGump `+a0` | 기존 `TextGumpNode::head` |
| 종류·이름·매개변수 | `+a4/+a8/+b8` | `kind/player/parameter` |
| 표시 기한 | `+b0` double, 초기 0 | `deadline` |
| 이름 선택 | `00540a24` 비영이면 `0048dc40`, 아니면 `00406020` | 현재 Challenge/Player 이름 장치 |
| 10초 기한 | `004ca270`: 현재 벽 시각 + float 입력 10 | 현재 `wallSeconds()` + 10, 기존 Idle 등록 |
| 유지 필드 | `+ac/+bc`는 이 생성자에서 쓰지 않음 | 기존 `reserved/trailing` 유지 |

아래 문자열은 CD PE의 `0053f338~0053f428`에서 직접 읽은 원문이다. 10.78의 `~E` 강조와 플레이어 색별 서식은 CD에 없다. CD 종류4는 금액을 서식에 넣지 않는다.

| 종류 | CD 문구 | 경고음 | 기한/Idle |
| --- | --- | --- | --- |
| 1 | `~wServer not responding` | 있음 | 0/등록 없음 |
| 2 | `~w%s not responding` | 있음 | 0/등록 없음 |
| 3 | `~w%s is present` | 없음 | 현재+10초/등록 |
| 4 | `~w%s sent you Storm Power` | 있음 | 현재+10초/등록 |
| 5 | `~w%s has disconnected` | 있음 | 현재+10초/등록 |
| 6 | `~w%s wishes to declare a Draw` | 없음 | 현재+10초/등록 |
| 7 | `~w%s and %s are now Allies` | 있음 | 현재+10초/등록 |
| 8 | `~w%s and %s have broken their Alliance.` | 있음 | 현재+10초/등록 |
| 9 | `~wGame started!` | 없음 | 현재+10초/등록 |
| 10 | `~wGame completed!` | 없음 | 현재+10초/등록 |
| 11 | `~wGame Paused!` | 없음 | 0/등록 없음 |

종류7/8의 첫 이름은 현재 Challenge 모드를 따르지만 두 번째 이름은 항상 일반 Player에서 조회한다. 두 번째 조회 뒤 번역하며, 첫 이름의 실제 바이트는 번역이 끝난 뒤 읽는다. 기존 10.78 생성 흐름은 별도 경로로 유지했다. CD switch에 없는 16(게임 시간)·17(Warning)·18(자원 지급) 등은 기본 TextGump를 만들기 전에 명시 오류로 남긴다.

## 검사 범위와 남은 검증

추가 집중 검사는 `MessageGump_CdPauseUsesCdTextVtableAndPermanentDeadline`과 `MessageGump_CdUnsupportedKindsPreserveUnconstructedNode`이다. 일반/Challenge Pause에서 CD 가상표·번역 키·0 기한·무음향·무Idle·현재 필드와 효과 순서를 확인하고, 미지원 종류가 전체0xc0 노드를 변경하지 않는지 확인한다. 실제 게임·패킷·통신 장치가 없는 UI 검사다.

이 작성 시점에는 **빌드·검사 실행 전**이며 루트 작업에서 통합 수행한다. 종류1~10은 원문에 맞춰 구현하고 정적으로 대조했으나 사용자 실행 제외에 따라 새 MP 관련 단위/패킷/통신 검사를 실행하지 않는다. 단순 CD 시작에서 이 차단이 해소되는지와 그다음 차단 지점은 통합 실행 후 기록한다. CD 튜토리얼 조립의 전체 판본 복원 완료를 뜻하지 않는다.

## 통합 검증 결과

Release `NetstormCpp`와 관련 집중 검사 대상 빌드가 종료 코드 0으로 완료됐다. 새 `MessageGump_Cd` 검사 2건과 기존 `MessageGump_PauseNative324`(원본 fixture 324관측)가 모두 통과했다. 10.78 일반 메뉴 40프레임 대조는 종료 코드 0이다. CD 사본은 기존 MessageGump 생성자 차단을 지나 `RawBattleCompletion`의 `일반 전투 종료의 실제9 Player/동맹/옵션 누락`으로 종료했다. CD의 현재 Player 초기화가 아직 연결되지 않았으며, 메뉴 표시 성공으로 기록하지 않는다. CDDir 검증과 보호 파일 해시 결과는 [같은 단위의 경로 문서](cpp-cd1072-configured-directory-20261114.md), 튜토리얼 후속 순서는 [설치 경계 조사](cpp-cd1072-tutorial-integration-audit-20261114.md)를 따른다.
