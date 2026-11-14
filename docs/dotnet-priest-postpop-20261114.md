# dotnetpj 0.506 — PATCH 사제 등록·회복 예약 접두

2026-11-14, HJOW-Athlon. AI가 조회하는 사제 목록의 원본 등록 몸체를 이식했다. 호스트의 전체 사제 가상 Pop·AI 생성 호출은 아직 연결하지 않았다.

## 구현

기준은 `9bea55e239a6574c0856a5e8349c8689167ebdf3`의 `o/RawPriestPostPop.h/.cpp`와 `PriestPostPopTests.cpp`다. C# `RawPriestPostPop`은 PATCH004950f0의 Carrier 호출 전 접두를 수행한다.

현재 extra&9이면 종료하고, 최초 호출일 때 signed 개수·용량에 따라 목록 중복 검사/등록→이벤트602 검색→할당 관문→현재 사제TYPE HP→객체 최대HP→예약을 순서대로 실행한다. 마지막에는 콜백 후 현재 원래 소유자·소유자BYTE를 다시 읽어 해당1..8 상태 한칸만 지운다. HP 분자 계산은 int/6 절삭→double50배→float 저장→객체HP 나눗셈 순서다. 누락된 실제 생성 효과 검사는 HP 조회 뒤에 한다.

기존 `RawPriestPreDestroy.cs`의 `PriestPostPopState`에 독립/공유 소유자9DWORD와 현재 getter를 더했다. 기존 사제 목록·GetPriests는 그대로 유지해 등록·조회·삭제가 같은 목록을 쓴다. 소유자 상태 초기화 함수도 유효 한DWORD만 쓰고, 무효 입력은 진단 뒤 쓰기를 생략한다. ProcessHost 어댑터는 cpp와 동일한 실제 검색/생성을 사용한다. 관리 객체 메모리 부족은 AddRegular에서 전파한다.

C# HP 장치가 아직 PATCH 전용이므로 이 접두도 PATCH만 허용한다. CD/10.37 관측은 같은 원본 파일에 보존하되 이번 재생 대상은10.78뿐이다. 전체 사제 PostPop 등록으로 연결하지 않아 Carrier·낙하·보호막 효과를 생략한 채 성공으로 처리하지 않는다.

## 검증

`priestpostpop-x86.tsv`는 주석 제외7176입력 중10.78의 두x87정밀도2392입력을 재생한다. 목록 전체/stale 꼬리·소유자9DWORD·실제 호출 순서·슬롯50바이트를 원본 결과와 전수 대조했다. HP/SP 장치는 실제 모듈이며 SP가 초기화되지 않는 것도 확인했다. fixture SHA256은 `3924ceb7ffb2315eae4fd8a9a3e89b9bff261994d36c0109ac684c1d89738cdd`다.

추가 검사는 할당 실패/기존 예약의 HP 조회 생략, HP 진단과 누락 AddRegular의 검사 순서, 콜백 뒤 공유 상태표 교체, 현재 사제TYPE 분자 재조회와 데이터 전용 초기화를 확인한다.

실제 사제/ProcessForm 생성자·HP·공유 목록·조회·커널 검사에서 payload8.25·시각11.5·부모 연결, 중복 예약 방지, 실행 후2초 재예약, 1/4HP 전환 뒤 기존 예약 보존·삭제 후payload8 재생성, 최종 커널/풀 수복을 확인했다. 이 검사는 공간 등록/소유자 접두 후 입력을 명시 준비하며 전체 게임의 사제 Pop 검증으로 세지 않는다.

신규 포함 집중20건이 통과했다. 나눗수6→7 변이는 원본356/2392 불일치와 실제 합성 검사payload8.25→7 실패를 검출한 뒤 원복했다. 원복 뒤 전체 비증분 Release 빌드는 경고0·오류0이며 관련집중24건을 재통과했다. 산출물은 `extracted/dotnet-priest-postpop-0506/`에 둔다. PORT_MAP3행 추가로707행(L1 346·port361), fixture1개 추가로244개다. 기존243개 해시는 보존했다.

새 게임 실행·포획/희생/공격/승리·MP 재현·신규 NB·외부 접속·기존 경로 은퇴는 없다. 보호 파일·cpppj·LEFT_JOBS.md·병행 webpj 변경은 보존했다. M1은0/34다.

## 다음

사제 후반 `RawPriestPostPopTail`을 이식해 접두→실제 Carrier→현재 Immobile/프레임/지면→낙하·보호막 순서를 연결한다. `RawPriestState`, `RawPathAnimation`, `RawCarrierPostPop`, `RawFrame`은 이미 있으므로 먼저 현재 계약을 확인해 재사용한다. 보호막 조회/생성·낙하 소비자가 미이식이면 같은 원본 모듈 묶음으로 채운다. 이어 공통 PostPop 가상 분배와 실제 요새 Pop을 연결한 뒤 AI 생성/초기 갱신을 조립한다.
