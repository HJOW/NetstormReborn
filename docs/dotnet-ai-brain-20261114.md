# dotnetpj 0.500 — 실제 AI본체 생성·소멸

2026-11-14, HJOW-Athlon. R4의 임시brain토큰을 실제 본체로 바꾸기 위한 첫 조각이다. **생성자 부품은 완료했고 라이브 호스트 연결은 아직이다.** 다음은 `RawPlayerTechnology`→`RawAiBrainInitialUpdate`→같은 세션·Player·섬·메타데이터 결선이다. 초기 갱신이 비어 있는 상태에서 `RawAiCreate`에 생성자만 붙여 성공으로 처리하지 않는다.

## 이식

기준은 `04c87c034`의 `cpppj/src/o/RawAiBrain.h/.cpp`와 `cpppj/tests/AiBrainTests.cpp`다. 새 `RawAiBrain.cs`에 실제 소유 블록 레지스트리·PATCH00416860생성자·00410100소멸자·00413ae0본체 해제를 옮겼다.

- 본체0xb08, 설정600바이트 복사,16개 포인터/용량/개수 목록과ae4의120바이트 목록을 원문 순서로 구성한다.
- 나머지 미초기화 바이트를 보존하며 원문 고정필드·double0·255DWORD배열만 쓴다. 미초기화 입력은 필수 공급자로 받는다.
- 같은 Player9·원본 방향별81DWORD동맹·RawIslandRuntime을 재사용한다. 사제 좌표 지역과 요구 에너지ss는 실제 조회 경계다.
- 무효 그래프는 두 쓰기 위치에서 현재값을 다시 읽는다. 예외에는 실제 생성한 목록만 역순 해제하고 상위 생성자가 본체를 해제한다.
- 목록 소멸과 본체 해제를 분리해 Participation의 기존 `DestructBrain`·`FreeBrain`에 연결할 수 있다. 가짜 토큰·중복 초기화·이중해제는 거부한다.

C#의 차이: 실제 포인터 대신32비트 불투명 토큰과 유일한byte배열을 사용한다. C++장기 전역 참조는 현재값 조회 대리자다. 토큰이 원본 실행 주소와 같음을 주장하지 않는다.

## 검증

기존 원본 `ai-brain-x86.tsv`를 복사하지 않고Core검사에서Link로 연결했다(SHA256 `53aecd3e635d5743ea07091d863d6db7fb328daa8181b7a5769b78a233087fab`, 헤더 포함161행·고유 입력160개). 원본 fixture는 바꾸지 않았다.

`AiBrainTests`4건은 cpp검사를 옮겼다.160입력의 본체 전체2824바이트·Player1620바이트·동맹324바이트·섬3204바이트·18블록별 토큰/크기/전체Adler·17목록의 역순해제·조회순서를 전수 대조한다. 별도로 실제 Participation부착/제거, 미연결초기갱신·메타데이터 거부/예외정리, 두 그래프 쓰기 사이BYTE변화, 생성 전 조회기 검사를 확인한다. fixture의SID32767도 원문 하네스처럼 할당상태를 바꾸지 않고명시 입력 좌표만 준다.

신규4건+`RawAiCreateTests`·fixture/PORT_MAP무결성의 집중14건은 통과했다. 대표 변이로 생성자+0x3d8의1을2로 바꾸면 `Native160`이160/160불일치로 실패했다. 변이를 원복한 뒤 전체솔루션 비증분Release빌드 경고0·오류0 및 집중14건 재통과를 확인했다.

PORT_MAP은2행 추가해690행(L1 340·port350), fixture목록은237개다. 새NB수정·외부접속변화·기존경로은퇴·게임창실행은 없다. AGENTS·originals·originalCD·README개요·cpppj·LEFT_JOBS.md 및 병행webpj변경은 보존했다.

## 회귀 기준선 종결

0.499에서 시작한 전체 회귀가 끝났다. Assets366·Core1945·Client1171, **합계3482·실패0·건너뜀0·종료0**이다. Core전체는10분34초가 걸렸으며 정지/중단 없이 완주했다. TRX는 `extracted/dotnet-regression-0499/`에 있다. 이 전체회귀는0.499코드 기준이고, 본 생성자는 위 별도 원본 전수검사로 확인했다.
