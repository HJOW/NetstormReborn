# 객체 Base·내장 Form 진단 서식 복원 — 단위555

2026-10-29 / HJOW-Athlon. 마지막 완료된 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 현재 PC와 같다. 디컴파일 산출물·원본/CD·AGENTS.md는 변경하지 않았다. 사용자 새 `/goal`로 재개한554의 재귀 부품에 이어 실제 가상+0x70의 선행 서식을 복원했다.

## 복원 범위

`RawObjectTraceFields`는10.78의 Base `004ac400`, Form `004ae1e0`, ProcessForm `004ae3f0`, DependForm `004ae530`, ContentForm `004aea90`이다. 공통Base는 실제TYPE+4 이름의 첫NUL·최대20BYTE와 next/head WORD·free/dead/void/contained 비트를 원문 색상 마크업/공백/LF로 기록한다. `005c8454!=0`일 때 next/head만 생략하고 파생Form의 부모/이전WORD는 그대로 출력한다.

파생 줄은 공통Base의 로그 콜백이 복귀한 **뒤** 현재 필드를 읽는다. ProcessForm은+0x12를signed DWORD로 읽고 `%03d`의 부호/0패딩을 보존한다. Form은+0x10 부모/+0xe 이전WORD, Depend는+0x12/+0x14의WORD 쌍이다. Content는 현재TYPE WORD/+0xc수량/+0x14소문자16진flags를 사용하며 `sprintf(TYPE.name,"XXX")` 후20BYTE를 출력한다. 실제 PE의 `00512a38` C문자열은 `XXX`다. TYPE명에 포함된 `%%`·한 개의 상수 너비/정밀도 `%s`를 보존하며 Base에서 같은 이름은 서식으로 해석하지 않는다. 원본이 만든 뒤 출력에 쓰지 않는 Content의 이전WORD 임시문자열은 외부 효과가 없어 별도 출력하지 않는다.

호스트는 물리 풀/TYPE 범위 밖 접근, Content 이름의80BYTE 임시버퍼 초과, 한 개를 넘는 CRT 인자나 미정의 변환을 예외로 거부한다. 이때 이미 전달한 Base 로그를 되돌리지 않는다. 임의 이름/TYPE0/부모0으로 대체하지 않는다. 원문의 임시버퍼 범위 밖 쓰기·미정의 varargs를 정상 출력으로 주장하지 않는다.

**실제 게임의 가상 분배·로그 장치·Ctrl Shift F7은 아직 연결하지 않았다.** 자산의 Main `004b0fb0`·Dude Task `00450d60`, 덤프 부모 `004d6020`, 현재Kernel/ThunderProcess, 실제 파일/키 연결은 후속이다. 게임 실행 파일은552 그대로이며 새 게임/원본/네트워크 실행은0이다.

## 원문과 검증

`tools/decomp_object_trace_fields.py`는 위 다섯 실제 몸체와 `0049a840` TYPE 조회를 실행한다. CRT sprintf·로그만 외부 경계로 대체하고, TYPE/필드/모드 분기와 호출 순서는 원문 기계어가 계산한다. 함수 코드 불변·정상 반환/ESP·x87 제어 워드/TOP·예외 목록 복구를 확인했다. **308고유 입력×두 정밀도=616관측, 차이0**이다.

고정 fixture는 다섯 본체×상태16조합×모드0/1/2, 이름 없음/긴 이름/20BYTE 경계/첫NUL/마크업/퍼센트/XXX치환, Form65535·PID0/999/1000/INT_MAX/INT_MIN/-1, 공통로그 콜백 중 파생 필드 변경을 포함한다. C++의 모든308행 출력이 일치했고 호스트 미정의 경계/로그 부재 검사도 통과했다. 이 단위 최초 검사 실패는 없었다.

최종 Release `netstorm_tests` 빌드 경고/오류0, ObjectTrace4+ProcessSummary2+Kernel5+Contained15의 **고유집중26개/실패0**, 등록3404다. 전체 C++/CTest·전체34경로·자연완주는 실행하지 않았고549의 자동 전술 실패/보류를 해제하지 않는다. 보호1820·기존fixture714개(합계2534)는 바이트 동일하며 게임552도 동일하다.

보고서는 `cpppj/object-trace-fields-20261029.json`, 원시는 `extracted/cpp-object-trace-fields-555-20261029/`의 `before.json`, `native.json`, `build-v1.log`, `build-v2.log`, `test-fields-v1.log`, `test-object-trace-final.log`, `test-summary-final.log`, `test-kernel-final.log`, `test-contained-final.log`다.

다음은 Main/Task의 현재 필드/가상 부모·이전/graph/island 서식을 복원하고554 재귀·실제 가상 분배/로그·덤프 파일·553 Kernel 요약을 연결하는 것이다. 전체 목표는 진행 중이며 기존 자연완주/사용자개입·스펠 도움말 그림·연녹색 직사각형·언어파일 부재 보류와 공중3종 정적 제한·MP 신규 검사 금지는 유지한다. 원격 push는 하지 않는다.
