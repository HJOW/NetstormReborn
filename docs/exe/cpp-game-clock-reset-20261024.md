# GameClock Reset00460e70 독립 복원 제안

현재 결과는 **미적용 사본의 새 3검사와 기존 시계 1검사, 합계 4검사/실패 0**이다. 격리 MSVC 첫 빌드·실행은 모두 종료 0이고 `/W4` 경고·오류는 없다. 공유 `GameClock.h/.cpp`, 공용 빌드·라이브러리·게임 실행은 변경하거나 실행하지 않았다. 실제 State 정리 부모의 ResetClock 연결과 전체 Client 시계 수명 검증은 후속이다.

## 원문과 API

- 현재 분석 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 원본 10.78 SHA는 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다.
- [함수표](../../extracted/refined/originals/functions.tsv:1753)의 `00460e70`은 13BYTE이고 호출자는 `004b2ebd`이다. [정밀 C](../../extracted/refined/originals/Netstorm.c:78662)와 실제 PE를 함께 확인했다. 이 함수의 CD10.72 SourceMap 주소는 없으며 CD 실행 증거도 없다. 인접 Game/Wall/Pause/Resume/Capture의 기존 CD 대응 주소로 Reset 주소를 추정하지 않는다.
- 실제 명령은 `FLD qword [00500460]`, `FSTP qword [0055b4f8]`, `RET`이다. PE 상수 `00500460`의 8BYTE는 모두 0이며 결과는 `+0.0`이다.
- 제안 API는 `void GameClock::Reset()`이다. 현재 대응 필드 `pausedTotal_`에 `0.0`만 쓴다. 원점, 정지 깊이, 정지 시 저장 게임 시각, 현재 Frame, 난수 상태, 현재 시각 입력은 요구하거나 변경하지 않는다. 정지 해제나 Capture도 호출하지 않는다.
- 기존 `GameSeconds`, `WallSeconds`, `Pause`, `Resume`, `IsPaused`, `Capture` 몸체와 32비트 밀리초 래핑 계산은 그대로다. 기존 C++가 생략한 원본 epoch `0055b4a4`, adjustment `0055b4ac`, FPS 통계와 Frame 번호의 기존 64비트 확장은 이번 Reset으로 복원됐다고 표현하지 않는다.

## 제품 작성 전 최초 관측

[독립 도구](../../tools/decomp_game_clock_reset.py)는 Reset 사본·새 검사가 존재하기 전에 [최초 TSV](../../cpppj/tests/fixtures/game-clock-reset-x86.tsv)와 [최초 보고서](../../cpppj/game-clock-reset-native.json)를 고정했다. 이 자료는 재작성하지 않는다. 최초 로그는 `extracted/cpp-game-clock-reset-20261024-native-first.log`에 있다.

- x87 제어어 `0x027f`·`0x037f`에서 **138관측 / Reset 46회 / 실제 명령 3,252개**, 두 정밀도 결과 차이는 0이다. 원본 전체 clock96BYTE, 난수 관련 네 DWORD의 16BYTE, 반환 비트, 시각 장치 호출 수, 스택·비휘발 레지스터·FPU 균형을 검사했다.
- **raw 12입력×2=24행**은 임의 비영 메모리, 극단 정지 깊이, 누적값의 NaN 비트 등을 포함한다. Reset 뒤 clock의 앞 88BYTE는 그대로이고 마지막 8BYTE만 0이다. 시각 장치 호출은 0이며 난수 관련 저장소는 유지된다. 이 부분은 원본 전역의 물리 불변 증거다. C++ private 객체 배치를 원본 전역 배치로 가장하지 않는다.
- **공개 호출열 6개×두 정밀도=114행**은 running/paused/nested/repeated/fresh/wrap 입력이다. 실제 원본 Game/Wall/Pause/Resume/IsPaused/Reset 몸체를 실행하고 `timeGetTime`만 명시 DWORD 장치로 제공했다. epoch·adjustment는 현재 C++ 표현 범위에 맞게 0이다. Reset 이외 몸체까지 단순 수식 기대값으로 대체하지 않았다.
- 실제 Capture/FPS 원문은 이 도구에서 실행하지 않았다. C++의 Frame 보존 검사는 기존 공개 Capture 동작의 회귀이며, 원본 Capture 전체 복원 증거와 구별한다.

## 사본 변경과 격리 검사

미적용 패치는 [제안 patch](../../extracted/cpp-game-clock-reset-20261024-proposed.patch)이며 사본은 `extracted/cpp-game-clock-reset-20261024-proposed/copy/`에 있다. 패치 순서는 기존 GameClock 두 파일에 무인자 Reset 선언·정의 추가, 신규 `cpppj/tests/GameClockResetTests.cpp` 추가다. 다른 제안이나 새 공유 소유자에 의존하지 않는다.

1. `GameClockReset_IndependentNative138AndPublic114`: 최초 138행을 모두 분류한다. 공개 114행은 실제 C++ 반환 double의 비트까지 원본 결과와 대조하고 raw24행은 원본 불변으로 별도 확인한다. Reset 무인자 서명도 고정한다.
2. `GameClockReset_PreservesOriginAndCapturedFrameHistory`: Reset 전 누적 정지 3초를 만든 뒤 같은 현재 시각에서 game 2→5, 다음 delta 3·Frame 번호 3을 확인한다. 반복 Reset은 delta0·번호4다. 원점 재설정, Frame 초기화·선행 Capture를 구별한다.
3. `GameClockReset_KeepsNestedPauseAndHeldGameTime`: 정지 깊이 2에서 Reset 후에도 held game1을 보존하고 마지막 Resume 이후 원래 누적 계산을 수행한다. 다시 Reset하면 game2→11·delta9이며 Frame 번호도 보존된다.
4. 기존 [RecoveryTests](../../cpppj/tests/RecoveryTests.cpp)의 `GameClock_NestedPauseAndWrap_AreReproduced` 본문을 수정 없이 추출해 같은 격리 실행에 포함했다. 추출된 본문과 원본 파일 SHA는 `isolated/existing-regression.json`에 있다. 원래 검사 파일과 등록은 바꾸지 않았다.

`isolated/build1.py`는 별도 폴더에서 MSVC 14.51.36231·SDK10.0.26100.0, `/std:c++20 /EHsc /utf-8 /W4 /permissive-`로 단일 컴파일한다. 공용 lib를 읽거나 링크하지 않고 `/MP`도 사용하지 않는다. 첫 `build1.log`·`test1.log`·`result1.json`은 그대로 보존한다. 공용 등록 시 새 파일의 3검사와 `NETSTORM_GAME_CLOCK_RESET_FIXTURE`의 최초 TSV 경로만 추가하면 된다. 기존 Recovery 검사를 중복 등록하지 않는다.

## 동결과 범위

`extracted/cpp-game-clock-reset-20261024-proposed/verification.json`은 공유 두 파일의 사본 작성 당시 SHA 보존, 별도 사본의 patch check/apply 결과, 최초 자료 SHA 검증, UTF-8·구문·공백 검사와 최종 파일 SHA를 기록한다. 현재 소스 동결은 이 제안 사본 범위이며 root 적용 이후 공유 소스가 달라지는 것은 별도 단계다. 게임·GUI·전체 CTest·State 부모 접합은 실행하지 않았다.

첫 patch 검사만 생성기의 CRLF 출력 때문에 적용 전 종료 1이었다. 최초 patch와 적용 전 사본을 보존하고 패치 출력 줄 끝만 LF로 교정했다. C++ 사본·검사·첫 빌드·최초 Native는 바꾸지 않았다. 최종 별도 `apply-check2`의 check/apply는 모두 종료 0이다.

동료 `dialog_lifetime`이 사본 세 파일과 원문 13BYTE를 읽고 의미·수명 오류를 찾지 못했다. 무인자 Reset의 한 필드 쓰기, raw24/공개114 증거 분류, Frame 전후·중첩 정지 검사의 범위도 확인했다. 이는 동료의 읽기 검토이며 추가 실행 증거가 아니다.
