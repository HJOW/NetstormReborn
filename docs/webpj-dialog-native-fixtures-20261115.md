# webpj 대화상자 선택/종료 명령과 초기 점 명령의 원본 관측 연결 (WB04-E467, 2026-11-15)

새 모듈이 없는 검증 단위다. E435(`DialogCommandLifetime`·`DialogExitCommand`)와 E437(`RawIssueCommands`)이 "cpppj에도 직접 시험이 없다"고 기록했지만 cpppj에는 직접 시험과 원본 관측 fixture가 있었다(E465/E466 감사, `docs/webpj-fixture-audit-20261115.md`). 이 단위가 세 fixture를 처음으로 연결했고 모두 **첫 실행에서 전체 일치**해 이식 모듈을 고치지 않았다.

## 연결한 관측

- **`dialog-command-lifetime-x86.tsv` 258행(발화 252행)** — `fireDialogCommand`가 선택 명령의 세 정수(command, parameter, parameter)를 같은 입력 큐에 먼저 예약한 뒤 원래 부모만 Kill하는 순서와 값. 입력 종류(즉시/해제) 비교 수(126/126)까지 대조한다.
- **`dialog-exit-command-x86.tsv` 204행** — `resolveDialogExitCommand`의 regex 캡처(명령/인자 hex, 두 정밀도, Config 코드 0 여부)→조회→진단→첫 BYTE `isdigit`/`atol`→문자열 고리→두 필드 저장 순서와 `disposeDialogExitCommand`의 한 번 예약. 새 모듈이 재사용하는 숫자 CRT 내부 관측(`Digit`/`Atol`)은 외부 효과 비교에서 제외한다(원본 시험과 동일).
- **`issue-commands-x86.tsv` 122행** — `registerIssueCommands`의 네 등록 호출(flash 중복 포함)과 `invokeIssueCommand`(flash/cheatorama)의 NULL/부호/32비트 감김·인용·78BYTE 토큰·기존 치트 전후와 외부 호출 순서.

## 정정

- `dialog-command-lifetime.js`·`dialog-exit-command.js`·`issue-commands.js`와 `issue-commands.test.mjs`의 "cpppj에도 직접 시험이 없다" 문구를 연결 사실로 바꿨다(주석 전용 변경).
- `PORT_MAP.tsv` 세 행의 메모에 원본 행 수를 더했다. manifest `dialog-command-lifetime`·`dialog-exit-command`·`issue-commands`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- 같은 감사에서 `tests/dialog-command.test.mjs`에 원시 NUL 바이트가 들어 있어 git이 이진 파일로 취급하던 것을 `\0` 이스케이프로 고쳤다(의미 동일, 시험 그대로 통과).

## 남은 것

`DialogCommandLifetime` 나머지 두 cpppj 시험(`CurrentQueueDeferredExit`)·`DialogExitCommand`의 `CurrentRingQueueAndSingleDisposal`/`MissingAndPhysicalBoundaries`는 기존 `dialog-command.test.mjs`가 같은 계약을 이미 시험한다. 감사 이월 목록(LEFT_JOBS 3.8절)의 나머지는 후속 단위에서 처리한다.
