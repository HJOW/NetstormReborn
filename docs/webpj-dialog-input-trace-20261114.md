# webpj HTML 대화상자 명령·입력 확인·특수 포인터 진단·현재 TYPE HP DialogExitCommand 외 (WB04-E435, 2026-11-14)

RawMenuControls 선행 소형 단위 다섯 개 — **DialogExitCommand 76줄·DialogCommandLifetime 36줄·HtmlInputDialog 36줄·SpecialPointerTrace 28줄·CurrentTypeHitPoints 22줄(todo→port)** — 를 새 모듈 [`www/ui/dialog-exit-command.js`](../webpj/www/ui/dialog-exit-command.js)·[`www/ui/dialog-command-lifetime.js`](../webpj/www/ui/dialog-command-lifetime.js)·[`www/ui/html-input-dialog.js`](../webpj/www/ui/html-input-dialog.js)·[`www/ui/special-pointer-trace.js`](../webpj/www/ui/special-pointer-trace.js)·[`www/core/current-type-hit-points.js`](../webpj/www/core/current-type-hit-points.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손으로 계산한 값, 실제 `ScriptStringRing`·`InputQueue`·`readRiftHitPoints` 연결이다. 원본 직접 관측 수치로 세지 않는다.

## 기준선 뒤 변경 (3.6절)

다섯 파일 모두 기준선 뒤 변경이 없다.

## 구현

- **`DialogExitCommand`(원문 c83b0)**: `resolveDialogExitCommand`는 명령(80)/인자(256) 캡처를 첫 NUL까지로 자르고 NUL 포함 버퍼를 넘기면 호출 전에 거부한 뒤 `lookup` → 코드 0이면 진단(미연결은 거부) → **첫 글자가 숫자이면 `atol`(DWORD 감김), 아니면 문자열 고리 `store` 토큰** → 모든 조회가 끝난 뒤 `{command,parameter}` 두 필드를 함께 교체한다(마지막 해석이 남음, 따옴표/공백/쉼표 불변). `disposeDialogExitCommand`는 코드가 있을 때만 큐를 요구하고 **보유값을 먼저 비운 뒤** `(코드,인자,인자)`를 한 번 예약한다(재진입에도 반복 없음, 코드 0은 예약 없음). 실제 큐 변형은 `pushCommand(code, first|0, second|0)`다.
- **`DialogCommandLifetime`(원문 c79b0)**: `fireDialogCommand`는 큐/Kill 경계가 모두 있어야 하며 선택 당시의 코드·인자를 먼저 예약하고(코드 0도 예약, 콜백이 선택 객체를 바꿔도 보존) 그 부모만 Kill 한다. 소멸/OnExit는 프레임 후반 Cleanup에 남긴다.
- **`HtmlInputDialog`(원문 004cfae0)**: 편집 자식의 `copyText`(256)를 마지막 칸 강제 NUL로 만들고 첫 LF에서 자른 뒤 **종료 NUL에서 앞쪽으로 `isspace`(int8)가 참인 동안만 지우며 맨 앞 칸은 검사하지 않는다**(정상 CRT에서는 지우지 않음). 모드가 1이 아니면 `atol`을 호출(반환값 무시)하고, 고정 80바이트 후속 섹션을 NUL까지(없으면 거부) 읽어 `format(섹션, 입력)`을 `tell`한 뒤 1을 반환한다. 설정은 즉시 바꾸지 않는다.
- **`SpecialPointerTrace`(원문 4da263..2e6)**: Ctrl이 눌렸고 special이 비영일 때만 사건 좌표를 역투영 → 단일 지점 탐색 → 각 후보에 직접 가상 70 → 다음 후보 조회를 순서대로 하며 중복 후보도 제거하지 않는다(경계는 도달 시에만 요구).
- **`CurrentTypeHitPoints`**: 번호로 선택한 TYPE 레코드의 최대 HP를 실제 `readRiftHitPoints`로 읽되 getNum은 **현재 표에서 같은 레코드의 위치**로 구하고(없으면 서로 다른 물리 표로 거부) 현재 표 조회는 호출마다 한다.

## 검사와 발견

새 `tests/dialog-command.test.mjs` 7건·`tests/html-input-dialog.test.mjs` 5건·`tests/special-pointer-and-type-hp.test.mjs` 5건 **통과**.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 1건):** 입력 큐의 명령 사건 코드는 `(code & 0x0100ffff) | 0x02000000`이므로 `0x0200006d`다.

**변형 확인:** 연산자/정수 상수 변형 69개(dialog-exit-command 28·dialog-command-lifetime 7·html-input-dialog 29·special-pointer-trace 2·current-type-hit-points 3) 중 68개를 잡았다(처음 생존 7개 → 선두 NUL·인자 '0'·짝수 인자 보강). 남은 1개는 `html-input-dialog`의 후속 섹션 반복 한 칸 초과로 어느 쪽이든 NUL 없음 거부라 동등 변형이다.

## 남은 경계

- 실제 `RawMenuControls`/HTML 부모/대화상자 조립과 실제 Console 입력은 별도 단위다. 앱의 대화상자는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
