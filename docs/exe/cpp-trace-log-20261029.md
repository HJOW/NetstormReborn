# 공유 trace 로그의 상태·출력 순서 복원 — 단위557

2026-10-29 / HJOW-Athlon. AGENTS.md와 최신 인수인계를 다시 확인했다. 마지막 완료된 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 현재 PC와 같다. 이전556의 커밋은 원문 대조/코드 변경을 포함한 진척이다. 이번에도 디컴파일 산출물·보호 원본을 변경하지 않았다.

`client/RawTraceLog`는 원본 `004c2630`과 직접flush `004c2610`의 공유 상태/출력 순서다. `TraceLogState`는 level의PE초기값29, 현재FILE, signed양수에서만 쓰는 DWORD 들여쓰기, stderr복제/즉시flush 관문과1024BYTE 버퍼를 소유한다. 출력 장치는 현재FILE/버퍼를 빌리고 복귀 뒤 원래 값으로 되돌리지 않는다. **현재 Client의 장기 전역/실제 파일·Chat 장치 연결은 후속이며 게임552는 그대로다.**

서식 완료 바이트와 **원문 format의** 선행마크업 BYTE수를 받는다. `%s` 문자열 인자에서 나타난~를 format 접두로 오인하지 않는다. level이음수이거나 현재limit보다크면 입력/장치 없이 복귀한다. 선택된 호출은 선행~쌍→들여쓰기→본문/NUL을 같은 버퍼에 쓰고 FILE0을stdout으로 바꾼다. 끝CR/LF를 모두NUL로 쓰고 필요할 때LF하나를 다시 넣으므로 NUL뒤stale BYTE도 원문과 같다. 그 뒤 console→조건부stderr→현재FILE→조건부flush 순서다.

console이나stderr 출력이 같은 버퍼/FILE/flush 관문을 바꾸면 다음 효과는 그때의 현재값을 다시 읽는다. console뒤FILE이0으로 바뀐 경로도 raw장치에0을 전달하며 stdout으로 재설정하지 않는다. 실제 OS장치는 NULL FILE을 명시 오류로 거부해야 하며 이를 raw순회의 임의 정상화와 구별한다. 직접flush도 현재FILE0이면 아무 장치 없이 반환한다.

`decomp_trace_log.py`는 `004c2630`, 게임formatter `004de490`, 원본CRT를 실제 기계어로 실행했다. OS의CRT thread조회만 기본C locale(`00543efc→00543ea8`, 단일BYTE분류)의 명시 입력으로 연결했고 `_isdigit`과 실제표/formatter 계산은 원문을 실행했다. `00434f40`, `fprintf`, `fflush`는 실제 인자/공유바이트를 기록하는 출력 경계다. 게임/창/OS API는 실행하지 않았다. 초기 관측 실패는 formatter의 BYTE쓰기허용 범위와 미초기화CRT TLS 경계를 확인한 것이며 제품/고정 기대값 실패가 아니다. 원시v2..v4로그를 보존했다.

**81고유 입력×두 정밀도=162관측**, C++전체 출력/최종전역/1024BYTE가 일치했다. level상하한, 접두0/2/4BYTE, signed양/음들여쓰기와1000공백, NUL/퍼센트/개행, stdout/stderr/명시FILE·복제/flush 조합, console/출력콜백의현재버퍼/FILE/관문 변경을 포함한다. 실제중첩 호출에서 바깥 출력도 같은 변경 버퍼를 읽는지, 장치누락/범위오류/필터된입력/직접flush도 추가 검사했다. 잘못된1024BYTE 초과나 접두는 호스트 예외로 거부하며 원문의 버퍼초과를 정상화했다고 주장하지 않는다.

Release검사target 경고/오류0, 신규4+ObjectTrace8+ProcessSummary2+Kernel5+Contained15의 **고유집중34/실패0**, 등록3412다. 전체C++/CTest·실제게임·원본·네트워크실행은0이며549의자동전술실패/기존보류를 해제하지 않는다. 보호1820·기존fixture718개(합계2538)와 게임552는 전후 바이트 동일하다.

보고서는 `cpppj/trace-log-20261029.json`, 원시는 `extracted/cpp-trace-log-557-20261029/`의 `before.json`, `native-v2..v5.log`, `native-final.log`, `native.json`, `build-v1.log`, `test-log-v1.log`, 네 `test-*-final.log`다. 다음은 현재로그전역을 객체덤프 `004d6020`의 BaseFile 경로/열기/닫기/재쓰기/실패 복귀 순서에 연결하고 실제 Client·Kernel/ThunderProcess·Ctrl Shift F7으로 잇는 것이다. 자연완주/사용자개입·스펠도움말그림·연녹색직사각형·언어파일부재 보류, 공중3종 정적 제한·MP신규검사 금지·push금지는 유지한다.
