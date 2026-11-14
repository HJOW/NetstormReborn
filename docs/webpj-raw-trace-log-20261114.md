# webpj 공유 trace 로그 RawTraceLog (WB04-E451, 2026-11-14)

기준선 `1f00a25a`의 `RawTraceLog.cpp/.h` 71줄을 `www/core/raw-trace-log.js`로 옮겼다. 기준선 뒤 변경은 없다. 실제 FILE 대신 호스트 출력 토큰과 명시 장치를 사용한다. 코드가 파일을 열거나 파일 로그를 생성하지 않는다.

`TraceLogState`는 레벨 29·최초 스트림/들여쓰기/복제/flush 0과 단일 1024BYTE를 소유한다. `writeRawTraceLog`는 레벨 필터를 입력 조회보다 먼저 적용하고 첫 NUL·원문 format의 선행 `~`쌍·그 뒤 signed 양수 들여쓰기·1023BYTE 한도를 처리한다. 실제 formatter 출력의 끝 CR/LF 전부를 NUL로 지운 뒤 개행 하나만 되살리며, 나머지 stale BYTE는 유지한다.

효과는 Chat 전달→현재 stderr 복제→현재 스트림 출력→현재 flush 순서다. 각 효과 복귀 뒤 공유 버퍼와 stream/mirror/flush를 다시 읽으므로 중첩 로그가 바깥 출력 내용도 바꾼다. 직접 `flushRawTraceLog`는 스트림 0이면 장치 없이 복귀한다. 누락 장치·물리 초과·NUL 부재는 명시 오류이며 앞선 효과는 되돌리지 않는다. 입력 문자열과 출력 훅은 원문 바이트 문자열 계약이다.

원본 `trace-log-x86.tsv` **81입력의 전체 1024BYTE·최종 전역·장치 인자/순서**가 첫 실행부터 일치했다. 해시는 fixture manifest의 `trace-log` 항목에 있다. 신규 검사 5건은 이 대조와 실제 중첩 호출·선택 장치 누락·필터된 입력 무조회·1023BYTE·NUL 뒤 꼬리·음수 들여쓰기·개행·현재 NUL 부재를 확인한다.

Node v26.13.0·v22.23.3 전체 각각 **4,663건 통과, 실패 0**이다. 대응표 984행 오류 0, fixture manifest 형식/해시/등록 검사도 통과했다. Node v24는 설치되지 않아 미실행이다. 실제 Client 장기 owner와 browser console 출력 훅은 RawMenuControls/Client 조립에서 연결한다. 브라우저에 없는 FILE flush 처리도 그 플랫폼 경계에서 결정한다. 이번 단위는 7차 공통 오류/예외 로깅 구조 전체 완료가 아니며 실제 게임/브라우저 실행은 없다.
