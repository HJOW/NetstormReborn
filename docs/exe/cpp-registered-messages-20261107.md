# 1060 — 설정 도구와 LAN 서버의 등록 메시지

10.78의 `NETSTORMOOGABOOGA` 등록 메시지를 실제 Client 창에 연결했다. 실행 제품은 **1060 / 11.01**, EXE 12,332,032바이트, SHA-256 `626ee478d2e680fe133ee53a42c712bdab7a3b9fbb742ebf46a8eb0943122aba`다. 직전 CP 단위1059는 `dcf0013e / 20261107-86 CP`다. 새 R 서버나 멀티플레이 전체 완료를 뜻하지 않는다.

## 원문 분류와 실제 연결

`00438dc0`의 팔레트 적재 뒤 같은 `RegisterWindowMessageA`를 호출한다. `RegisteredMessageState`가0054da94의 ID와0054da98의 도구 표식을 최초0으로 소유하며 월드/Session 교체에서 초기화하지 않는다. 실제 `HandleMessage`는 같은 ID에서만 부품을 호출하고 일반 WndProc/DefWindowProc 처리를 계속한다. lParam 하위 DWORD의 `ff00h` 마스크와 다음 순서를 보존했다.

| 값 | 동작 |
| --- | --- |
| 100h | NULL 명령줄 설정 재적재→명시 InterpretOptions→등록 ID 현재 조회/동기화 |
| 200h | 도구 활성 표식1 |
| 300h | 도구 활성 표식0 |
| 400h | 게임 수신부에서는 별도 처리 없음 |
| 500h | 현재 실제 Screen bit0이 켜졌을 때만 같은 수신 창에 SW_RESTORE |

첫 시작과100h는 같은 `LoadNativeConfiguration`을 사용한다. 기존 파일/CD/환경 탐색·설정 Load·FileSpec 핸들 수명과 제품 표시11.01을 유지한다.100h는 시작 명령줄을 재적용하지 않고 `SaveOptions`를 추가하지 않는다. 최초 OS 언어 감지는 기존 위치에서 한 번만 수행한다. 현재 복원된 부분 옵션 해석을 공유한 것이며 전체00435220 복원 완료를 주장하지 않는다.

004b3410은 `registeredSubId`가-1인지 확인하고 같은 키를 다시 읽은 뒤 이름 NULL로004b32f0을 호출한다. 은퇴한 외부 서버 유효성 검사가 아닌 현재 Fort의 로컬 Subscriber 동기화다. Uber 생성 전에는 저장소/manager가 없고, 생성 뒤에는 같은 현재 소유자/9개 헤더의 manager→fort 모드 관문을 확인한다. 실제 Session이 있으면 기존 `ChangeFortSubscriber`의 같은 현재 handle/Subscriber writer/등록/Changed를 재사용한다. **manager와 fort가 살아 있으나 Session이 없는 메타 경로는 기존 RequireSession 명시 오류다. 비Session Template 저장/Changed의 장기 장치는 후속으로 남는다.** ID만 쓰고 저장 완료로 처리하지 않는다.

00435ad0의 현재 getter와00435ae0의 동기 `SendMessageA(HWND_BROADCAST, 현재ID, 0, 400h)` 장치도 마련했다. 실제 일반 명령 호출자는 아직 연결하지 않았다. 원문004d62b0의 이 표식 소비는 특정 명령 분기다. **메인 루프 정지 조건으로 사용하지 않았다.** R00479760의500h 통지는 서버 초기화보다 먼저 나오므로 로그인 준비 완료로 해석하지 않는다.

## 검증과 잔여

첫 Release 빌드는 UI owner 이름을 잘못 적은 한 원인/오류2로 종료1이었다. 실제 멤버 `menu_`로 교정한 최종 게임 Release/x64는 종료0·경고/오류0이다. 새 단위/MP/게임/통신/원본R/실제 등록 메시지 송신 검사는 모두0이다. 보호2800파일 불변·새 fixture0이며 전체 검사/34미션/ZIP을 반복하지 않았다. 원시 코드 사본과 빌드 로그는 Git 제외 `extracted/cpp-resume-1060/`, 기계 판독 기록은 `cpppj/registered-messages-20261107.json`이다.

전체 목표는 미완료다. 비Session 등록 동기화·일반 명령 소비자, 새 R.exe/실제256칸 전송·OS 실행/Root·CS,17h/1dh·28~33/메타·LAN/Outpost/checking5가 남는다. 이후 한국어 후속→Edit 시나리오 UI 순서다. MP 동작 검증은 계속 제외한다. 마지막 전체 클라이언트 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 이번에는 다시 수행하지 않았다.
