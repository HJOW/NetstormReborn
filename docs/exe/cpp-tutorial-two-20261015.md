# cpppj Early Mission2의 전체 단계 조건 복원

2026-10-15, 현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex**다. 기존 정밀 C와 보호 10.78의 실제 바이트, 읽기 전용 `--read-game originals d/tutorial2.english`의 원본 스크립트를 확인했다. 새 전체 디컴파일·원본 파일 수정·실제 게임/GUI 실행은 없다. 전체화면 DirectDraw 오류 수정은 제외한다.

`RawTutorialTwo`는 PATCH `004c3bb0`의 A~H 조건과 부수효과 순서를 복원한다. 단계와 임시 타이머는 `RawMissionFrameGate::State()`의 **같은 `stage/timerSeconds`**만 사용하고, 제작 수/워크샾 허용은 기존 `RawTotalMade`의 실제 장부를 사용한다. 선택 분류는 현재 raw SID 슬롯의 타입 BYTE와 공유 TYPE의 genus를 읽으며 별도 객체/선택 분류를 캐시하지 않는다.

## 실제 조건과 순서

| 단계 | 원본 조건/효과 |
| --- | --- |
| A | 현재 옵션의 BYTE4=0·BYTE2=2 → 공급 반지름 재계산 `004b4860` → 신전 genus `0x200`의 누적 제작 수>0이면 Advance |
| B | 현재 썬 워크샾 타입(`005411ac`, 현재85) 허용 → 워크샾 genus `0x4000` 제작 수>0이면 Advance → 선택 초기화 → 선택 해제 |
| C | 선택 조회의 첫 결과가 비제로일 때 **한 번 더 현재 선택을 조회**하여 그 raw TYPE genus에 `0x200`이 있는지 검사. timer0이면 현재 게임 시각+2초 예약. 이미 예약됐고 기한에 도달했으면 `NotVortex` Tell → 성공 시 팝업 정리·미션 정지 → 선택 초기화/해제. 그 뒤 현재 썬 디스크 발사기 타입(`00541174`, 현재71)이 생산 목록에 있으면 Advance |
| D | 현재 썬 디스크 발사기의 두 번째 로컬 누적 제작 수>0이면 Advance |
| E | 같은 제작 수>1이면 Advance → 선택 초기화/해제 |
| F | timer0일 때 현재 신전 선택이면 현재 게임 시각+4초 예약. timer가 이미 비제로면 **선택을 다시 검사하지 않고** 기한 도달에서 Advance → 선택 초기화/해제 |
| G | 같은 제작 수>3이면 Advance → 선택 초기화/해제 → timer0 |
| H | 회수 금지 DWORD `00595078=0`을 매 호출 쓴다. timer0일 때 상위 비트를 제거한 현재 사건이 `0054286c` 회수 명령이면 현재 게임 시각+2초 예약. 이미 예약됐다면 다른 사건에서도 기한 도달에서 Advance |

반환값은 모든 단계에서0이다. I는 원본 `[I.]` 성공 안내·`<$Config,Done{mission.fileName}=1>`·Leave/Next 버튼 본문이 처리하며, 조건 함수는 I/범위 밖에서 아무 단계 효과도 실행하지 않는다. 이 기본 분기를 지원되는 큰 하위의 빈 성공으로 바꾼 것은 아니다.

실제 `004c3bda mov BYTE[0052f7e4],0`과 `004c3be1 mov BYTE[0052f7e2],2`를 확인했다. 정밀 C가 첫 전역을 DWORD로 표현하더라도 주위 옵션 네 바이트를 지우면 안 된다. 독립 검사에서는 옵션8바이트 전체를 비교한다. ROM `00506590`은 double2초, `00506588`은 double4초이며 현재 시각도 double을 유지한다.

모든 타이머 도달 조건은 포함 경계 `timer <= now`다. C에서 신전 선택을 해제해도 기존 timer를0으로 지우지 않는다. C의 성공한 NotVortex는 실제 미션 정지를 먼저 잡으므로 같은 호출 뒤의 생산 조건이 참이어도 `RawMissionFrameGate::Advance`가 정지 상태를 읽고 단계 증가를 거부한다. B/E/F/G의 선택 정리는 Advance가 정지/잠금으로 거부돼도 실행하며, G의 마지막 timer0도 유지한다.

## 실제 하위와 명시 경계

기존 `RawTotalMade::Allow/BuiltForGenus/Built`와 `RawMissionFrameGate::Advance/PauseForDialog`를 직접 사용한다. 선택 genus는 `004d5430` 두 번→`004ac200`→`0049a840`의 현재 raw/TYPE 조회를 같은 저장소에 적용한다. host SID/TYPE 물리 범위를 벗어나면 명시 오류를 낸다.

옵션 공급 범위 갱신 `004b4860`, 선택 초기화 `004d5c90(0,0,0)`와 후속 `004d5c60`, 생산 목록 조회 `0044f970`, 실제 Tell `00482de0`/팝업 정리 `00466710`은 필수 훅이다. 생성자는 누락된 훅을 거부한다. 공급 범위/생산·UI 전체를 복원한 것처럼 빈 callback을 넣으면 안 되며, 실제 상위 연결 전에는 미복원 경계를 예외로 드러내야 한다.

`004d5c90`와 `004d5c60`은 단순한 숫자 선택0 쓰기 이상의 실제 입력/Gump 효과를 가진다. 이번 모듈은 원본 두 호출 순서만 복원하며 호출자가 그 실제 몸체를 연결해야 한다. 단일 선택0으로 합쳐서 완료했다고 주장하지 않는다.

## 독립 원본 검증

`tools/decomp_tutorial_two.py`는 보호 PE의 Tutorial2 실제 몸체, 기존 네 gate 함수 범위, 현재 선택 getter/TYPE genus getter, 기존 선택 export의 Allow/Built/BuiltForGenus/TYPE getter를 실제 기계어로 실행한다. 함수 바이트 교체는 없고 허용하지 않은 함수/OS 진입 및 쓰기를 거부한다. 옵션 범위 갱신·선택 초기화/해제·생산 목록·Tell/팝업·시계 깊이만 명시 입력/관찰 경계다. 직접 원본 게임을 실행하지 않는다.

새 고유 입력은 **346개**이며 모두 정상 반환했다. 실제 몸체 진입은 Tutorial2 **346**, Advance166, Pause11, Allow24, 부류별 제작 합계49, 타입별 제작96, 선택190, genus77회다. 정상 반환 EIP/ESP·비휘발 레지스터·x87 제어어027f/TOP0을 확인한다. A~H/I/범위 밖, 제작 수 음수/0/1/2/3/4/INT_MIN/MAX, 정지/잠금, 기한 직전/동일/직후, 선택 없음/다른 타입/신전, 생산 유무, Tell 성공/실패, 옵션 주변 BYTE 보존, 외부 효과 뒤 재조회/최종 쓰기를 포함한다. 고정밀 현재 시각16,777,216.25초의2/4초 예약도 검사한다.

`TutorialTwoTests.cpp`는 같은 입력에서14필드 전체 게이트 상태·옵션8BYTE·회수 금지/워크샾 허용·현재 선택·정지 깊이·외부 호출 순서·반환을 직접 대조한다. 새 두 검사는 Native346행 대조와 필수 경계 누락 거부이며, 루트의 통합 Release 빌드는 경고/오류0, 집중2개·실패 체크0이다(`extracted/tutorial-two-build.log`). 이후 G의 예약 타이머 입력을17초로 강화한 fixture는 같은346고유 입력 범위이며 최종 재검사 결과는 상위 작업 기록을 따른다. 검증 자료는 `cpppj/tests/fixtures/tutorial-two-x86.tsv`, `cpppj/tutorial-two.json`, `extracted/tutorial-two/assembly.txt`다. `--verify`는 도구/원본/기존 함수 범위/정밀 C/fixture/어셈블리 해시와 입력 계획·정상 반환 수를 재검사하며 원본 관찰 수에 다시 더하지 않는다.

## 남은 실제 연결

현재 실제 미션의 옵션·선택·생산 장부·기술 허용·건설/수확과 gate의 전체 미션 프레임에 연결해야 한다. Tutorial2 헤더의 `denySalvage=1`, `myStartMoney=10000`, `myTech=sunArcher`, 처음에는 `windVortex;sunArcher`만 허용하는 시작 조건, B에서 워크샾 허용/H에서 회수 허용하는 흐름을 함께 보존해야 한다.

이 단독 복원은 실제 Tutorial2 완주, 전체 Early Missions/캠페인1-1~1-2, 실제 게임 음향/그래픽/UI 동일성, 성공 저장/다음 미션 진행의 검증이 아니다. CD10.72 대응은 미확정이다. 보호 PE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이며 신규 디컴파일 소스 변경은 없다.
