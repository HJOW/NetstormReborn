# State 정리 자식004c1f20과004b2b50 분류

현재 결과는 신규 `RawClockSyncReset`의 **독립 원문 64관측 대조·격리 2검사/실패 0**이다. 두 주소는 역할과 저장소가 달라 별도 경계로 유지한다. 기존 공유 소스·CMake·공용 빌드·라이브러리·GUI·git·인수인계 파일은 변경하지 않았다. 공통 State 정리 부모의 실제 Client 접합 완료로 세지 않는다.

## 원문 주소와 현재 대응

| 10.78 주소 | 실제 몸체 | 현재 제품과 복원 범위 |
| --- | --- | --- |
| `004c1f20` /18BYTE | `005c8bf4=0` → `005c8c00=0` → `0055b4ac=0`, 반환 | 이번 신규 모듈. 명령 큐 제거 함수가 아니며 인접 동기화 기록·시계 전체를 지우지 않는다. |
| `004b2b50` /10BYTE | `ECX=00542508` → `JMP004b28f0` | 기존 `SquidHash`의 공간해시 초기화 wrapper. 새로 복제하지 않는다. |

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 보호 원본 10.78 SHA는 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. [함수표](../../extracted/refined/originals/functions.tsv:3411), [정밀 C](../../extracted/refined/originals/Netstorm.c:154520)와 실제 PE를 함께 읽었다. `004c1f20`의 실제 명령은 `XOR EAX,EAX` 뒤 세 절대주소 `MOV DWORD,EAX`와 `RET`의 5개다. 분기·외부 호출·할당·난수·현재 시각 조회는 없다.

CD10.72의 SourceMap 대응은 `004ec490` /18BYTE이며 정밀 C에서는 `00546200`, `0054620c`, `0050f24c`를 같은 순서로 0에 쓴다. CD는 이번에 실행하지 않았다. 공간해시 wrapper의 CD 대응은 별도 `00479a50`이고 그 자식은 `00479ad0`이다. CD의 같은 숫자 `004b2b50`은 다른 함수이므로 주소만으로 혼동하지 않는다.

## 세 DWORD의 의미와 수명 경계

- `005c8bf4`는 동기화에서 사용하는 현재 연결 token이다. `004c1eb0`이 인자로 받은 연결을 저장하고 `004c2070`은 그 현재 객체의 가상 `+4`로 32BYTE 패킷을 보낸다. 연결 종료 경로도 같은 token과 비교해 0으로 지운다. 새 C++ 모듈은 이 token의 수명을 소유하거나 실제 연결을 해제하지 않는다.
- `005c8c00`은 인접 `004c1e70`에서 1, `004c1eb0`에서 0으로 설정한다. `004c1f60`은 비영일 때 실제 게임 시각을 패킷에 넣어 즉시 응답하고, 0일 때 벽시각과 기록표를 사용한다. 편의상 `replyMode`로 명명하며 큰 패킷 분배 부모를 복원했다고 주장하지 않는다.
- `0055b4ac`는 기존 [LocalMissionStateGlobals](../../cpppj/src/o/RawLocalMissionState.h)의 `gameTicks`와 [SavedStateReadGlobals](../../cpppj/src/o/RawSavedStateRead.h)의 마지막 tail 출력이 가리켜야 하는 같은 DWORD다. 실제 시계는 이 값을 시각 보정에 사용한다. 기존 `GameClock`의 정상 시작 범위에는 이 원본 DWORD가 없으며, 앞선 `GameClock::Reset` 제안의 `pausedTotal_`과도 다르다. 같은 owner의 실제 참조를 나중에 제공해야 한다.

현재 `ClientLocalStateGlobals`는 다섯 State 블록과 기한·두 명령 DWORD만 소유하며 이 연결 token·응답모드·`gameTicks`의 장기 owner는 제공하지 않는다. `RawLocalMissionState`·`RawSavedStateRead`의 참조/getter 계약이 존재하는 것과 공통 Client의 실제 저장소에 연결된 것은 구별한다. PE에서 이 세 주소는 `.data`의 가상 0 영역에 있지만, 이번 모듈이 임의 새 0 저장소를 만들지는 않는다.

## 기존 공간해시 경계

실제 `004b28f0`은 현재 선택 단계를 먼저 보관하고 0→3단계에서 현재 scale·폭을 기록한다. 각 배열이 NULL일 때만 원문 allocator를 호출하고 현재 폭²×2BYTE를 0으로 지우며, 단계0 배열을 `005c84bc`에도 기록한다. 마지막으로 유효한 이전 단계를 복구한다.

[SquidHash::Reset](../../cpppj/src/o/SquidHash.cpp)은 이미 소유한 네 WORD 배열을 같은 순서로 0에 쓰고 선택 단계를 유지한다. 기존 [HashTests](../../cpppj/tests/HashTests.cpp)는 독립 Init 자료와 선택3·배열 주소 보존을 검사하며 [hash.json](../../cpppj/hash.json)의 기존 Init 입력은 4개다. 이것은 정상적으로 할당된 고정 크기 C++ 배열을 재사용할 근거다. 원본 NULL 배열의 지연 할당, 임의 크기·scale/잘못된 선택값, 실제 `00542508` DWORD header와 `005c84bc` 포인터 전체 alias까지 같은 물리 부모로 복원한 증거는 아니다. 이번에는 기존 해시 몸체나 검사를 변경·재실행하지 않았다.

## 제품 전 독립 관측과 신규 모듈

[독립 도구](../../tools/decomp_clock_sync_reset.py)가 제품 헤더·본문·검사 파일이 없는 상태에서 [최초 TSV](../../cpppj/tests/fixtures/clock-sync-reset-x86.tsv)와 [최초 보고서](../../cpppj/clock-sync-reset-native.json)를 고정했다. 16입력×x87 `0x027f`·`0x037f`×반복2의 **64회 정상 반환 / 실제 명령 320개 / 정밀도 차이 0**이다. 외부 자식은 없으며 18BYTE 전체를 실행한다.

동기화96BYTE·clock96BYTE·별도 난수 관련16BYTE를 기록했다. 임의 비영 인접 메모리와 0/1/전체 비트1/최상위 비트/포인터 모양 DWORD를 제공했고 실제 세 store의 주소·폭·값 순서를 매번 비교했다. 인접 송신·수신 카운터, 시계 원점·정지 상태·현재 Frame, 동기화 기록·시각, 난수 관련 저장소는 보존됐다. cdecl 스택·EAX0·ECX/EDX/비휘발 레지스터·FPU 제어어/상태/tag도 확인했다. C++ void API가 EAX 반환 계약을 제공한다고 확대하지 않는다.

첫 도구 실행은 Unicorn이 `bytearray` 입력을 거부해 **CPU 진입 전에 종료 1**이었다. 최초 로그 `extracted/cpp-clock-sync-reset-20261024-native-first.log`를 보존하고, 가상 메모리 입력을 `bytes`로 변환한 뒤 첫 CPU 관측을 고정했다. 성공 로그는 `...-native-first-observation.log`다. 관측 후 최초 도구·fixture·보고서를 재작성하지 않는다.

신규 [RawClockSyncReset.h](../../cpppj/src/o/RawClockSyncReset.h)·[본문](../../cpppj/src/o/RawClockSyncReset.cpp)은 `ClockSyncResetGlobals`의 세 실제 `uint32_t&`를 `ResetClockSynchronization`에 전달한다. 바인딩 자체에 초기화가 없고 실제 호출만 세 필드를 순서대로 쓴다. 숨은 owner·세션별 0 복제·명령 큐 초기화·큰 동기화 부모를 추가하지 않는다. 호출자는 같은 저장소가 호출보다 오래 살도록 보장해야 한다.

## 검사와 최종 동결

[ClockSyncResetTests](../../cpppj/tests/ClockSyncResetTests.cpp)의 두 검사는 최초 64행의 동기화/clock 전체 BYTE와 실제 참조 재사용·인접 카운터 보존을 확인한다. `NETSTORM_CLOCK_SYNC_RESET_FIXTURE`에 최초 TSV 경로를 제공하면 된다. 필요한 제품 소스는 신규 `.cpp` 하나뿐이며 다른 신규 모듈에 의존하지 않는다.

별도 `extracted/cpp-clock-sync-reset-20261024-isolated/`에서 MSVC 14.51.36231·SDK10.0.26100.0, `/std:c++20 /EHsc /utf-8 /W4 /permissive-` 단일 프로세스로 컴파일했다. 첫 빌드는 독립 Runner가 존재하지 않는 `netstorm::tests::RunAll`을 사용해 종료 2였다. 이 실패의 Runner·로그·결과를 보존하고 실제 `netstorm::test::Registry` 순회로 Runner만 교정했다. 제품·검사·원문 기대는 그대로다. **최종 build2 종료 0·경고오류 0, 두 검사/실패check0**이며 공용 lib는 링크하지 않았다.

동료 `theme_technology`가 신규 세 파일을 읽고 참조 수명·store 순서·Native 배치와 큰 부모 미연결 범위에 명백한 오류를 찾지 못했다. 이 읽기 검토는 추가 실행 증거가 아니다. 최종 SHA·UTF-8/구문·최초 자료 검증·실패 이력은 `extracted/cpp-clock-sync-reset-20261024-isolated/verification.json`에 있다. StateCleanup의 `Clear4c1f20`은 실제 Client owner가 마련된 뒤 이 함수를 연결해야 하며, 현재 enum이 있다는 사실만으로 연결 성공을 판정하지 않는다.
