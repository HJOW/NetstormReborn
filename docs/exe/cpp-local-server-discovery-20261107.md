# 1057 — 원본 로컬 서버 정밀 분석과 검색 몸체

원본 `originals/R.exe`를 클라이언트 양판본과 분리해 정밀 디컴파일하고, `66h` 패킷·검색 토큰·두 기한·발견 수신의 원문 몸체를 제품 라이브러리에 추가했다. **실행 제품은1057 / 11.01이며 실제 사용자 흐름은1056과 같다. UDP 장치/큰 State34~38 및 새 로컬 서버 실행 대상은 아직 미연결이다.** 멀티플레이 완료나 동작 확인으로 계산하지 않는다. 직전 CP 커밋은 `6bff48b8 / 20261107-78 CP`다.

## 디컴파일 범위와 재개

마지막 클라이언트 양판본 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon** 그대로다. 이번에 추가한 **R.exe 전용 정밀 디컴파일은2026-11-07 / HJOW-Athlon**이며 별도 `NetstormRootServer` 프로젝트를 쓴다. 클라이언트 프로젝트/결과를 덮어쓰지 않았다. 원본 서버는 실행하지 않았다.

재생성 명령은 `powershell -ExecutionPolicy Bypass -File tools/ghidra/refine_root_server.ps1`이다. 결과는 Git 제외 경로 `extracted/refined/root-server/`의 raw.c/함수·규약·복구·switch 표와 ghidra 프로젝트다. 최초에는 임포트하고 이후에는 기존 프로젝트를 재사용한다. 명시 `-Reimport`만 재임포트한다. 원본 SHA-256은 `831e2ecbbba4654b97a270da8da91d7aa7302090f3198c4365f78c44d59dbf0b`이고 614,400바이트다. 다른 PC에 이 로컬 결과가 있다고 가정하지 않는다.

첫 분석에서 점프 스텁의 호출자를 놓쳐 미정 규약979개가 남았다. `ApplyConventions.java`에 선택적 `follow-thunks`를 추가해 Ghidra가 이미 확인한 thunk의 실제 호출/데이터 참조와 통과 호출·스택 프로브 대상을 추적했다. **기존 클라이언트 실행에는 옵션을 주지 않으므로 기존 판정 흐름을 유지한다.** 뒤이어 `clear-analysis-parameters` 옵션으로 자동 fastcall 인자가 thiscall의 가짜 스택 인자로 남는 추정만 해제한다. IMPORTED/USER_DEFINED 원형은 보존하고 새 인자 목록은 잠그지 않는다. 서버 전용 스크립트에서 두 옵션을 명시한다. 최종447300/475e60/475f90에서 가짜 `unaff_EDI` 인자가 해소됐음을 확인했다.

최종 함수5308개를 모두 내보냈고 디컴파일 실패0이다. 비thunk 규약 집계는 `{"__cdecl": 1581, "__fastcall": 2, "__stdcall": 46, "__thiscall": 1043, "unknown": 184}`다. 미정184개 및 가상 호출 원형은 추가 기계어 대조 대상이며, 성공 수를 원형 전체 검증으로 해석하지 않는다. `run_script.ps1 -Edition root-server -Refined`와 `export_functions.ps1 -Name rootdiscovery -Refined`로6함수를 읽기 전용으로 별도 내보낼 수 있다. 함수 목록은 `tools/ghidra/rootdiscovery-functions.json`이다.

## 복원한 검색 부품

`RawLocalServerDiscovery`의 저장소는 실제 장기 전역을 빌리는 참조형이다. 여기서 임의 난수/별도 성공 상태를 만들지 않는다. 후속 실제 Client 연결에서는 같은 `ClientRandomGlobals.Crt()`와005c85a8/005c8600/005c8608을 사용해야 한다.

| 원본 위치 | 준비한 몸체 |
| --- | --- |
| Netstorm0040ae40 /0052ee56..59 | version1·type66h·길이12, +4명령/+8값의 little endian 작성 |
| Netstorm004b4050 | 실제 검색 열기→현재wall×1000의_ftol하위DWORD로srand→rand4회의lowBYTE를상위부터조립→토큰저장→첫검색송신 |
| Netstorm004b3fc0 | 명령0/현재토큰을현재UDP브로드캐스트장치에인계. IPX는TCP/IP대상의범위밖 |
| Native State34 | 검색안내→첫검색→새wall+5→rand1회의0.500~0.599초double저장→다른새wall과합→chalPal→즉시35 |
| Native State35 | 재송신기한을먼저검사/현재토큰재송신→새지연/새wall. 검색만료때만기한0→실제서버창조회→없으면즉시37/다음35,있으면새wall+4/다음35 |
| Netstorm004b8a10 | 명령1은현재36을먼저쓴뒤현재sender주소를조회해current+8에NUL복사→packet+8을current+0x30DWORD→실제큰부모호출. 그외는**signed**현재토큰보다값이작을때만새wall+12 |
| R00475e60 | 실제UDP노드가선택됐을때명령1/현재TCPbasePort의같은12BYTE를세번브로드캐스트 |

검색 응답의 값은 클라이언트 검색 토큰이 아니라 서버TCP포트다. R00475f90의실제명령0응답분기는004d21b8의bit2를검사하며 R00477950의`rootMode` writer와함께읽어야한다. 출력문에적힌모드명만으로비트를가정하지않는다. R00477e10의실제UDP/IPX열기와로컬서버모드/Root·CS관리자복원은후속이다. 이번 `AnnounceLocalServer`는 이미 선택된 실제UDP장치를 받아 세번송신하는 부품이며 서버를 실행하거나 성공 응답을 합성하지 않는다.

Native `435ab0`은 `EnumWindows`로 제목에 `Netstorm Root Server`가 있는 창을 찾는다. 이를 진행 상태 플래그로 대체하지 않았다. Native38의 `e1040`은 자기 호스트의 실제IPv4를 구한다. 아직 새 로컬 주소 장치를 붙이지 않았으며 localhost나 기본 폐기 서버를 임의로 성공 처리하지 않았다. 수신 문자열은 현재144BYTE의 실제 물리만 허용한다. Native와 같은 작성 순서를 유지하므로 주소가 겹친 포트는 뒤의DWORD쓰기에서 갱신된다. x87 중간 정밀도의 모든 비트 일치와 실제OS실패는 별도 한계다.

## 확인과 남은 작업

같은 제품 라이브러리의 Release/x64 컴파일을 세 번 수행했고 최종 종료0·경고/오류0이다. 두 번째는 R 원문의 세 번 응답 작성기 추가 뒤이며, 최종은 State34/35의 즉시 상태를 별도 Request 진단 없이 원문처럼 직접 쓰도록 대조한 뒤다. EXE는12,292,096바이트, SHA-256 `0d415d68e4eb7713284ca2abeb5237a14a0be88f5f8dc427001eae37523c1745`이다. 정적 Ghidra/PE/원문 대조를 했고 **C++검색/패킷함수 실행·새 단위/MP/게임/통신 검사 모두0**이다. 전체솔루션/34미션/ZIP을 반복하지 않았다. AGENTS.md·원본/CD/fixture2800파일불변·새fixture0이며 DN/WB 작업은 포함하지 않는다. 기계 판독 기록은 `cpppj/local-server-discovery-20261107.json`이다.

다음은 같은Client장기전역/CRT와실제UDP12·공유패킷표/slot4수신·State34~38/새로복원한로컬서버실행대상을연결하는일이다. 원본R.exe실행을새제품의서버복원완료로대체하지않는다. 이후17h/1dh응답→State28~33/메타→나머지LAN/Outpost/checking5→한국어후속→Edit시나리오UI순서이며 전체목표는미완료다. 사용자제보C2607은1052의기존7검사통과로해결됐고그결과를이번MP검사로재분류하지않는다.
