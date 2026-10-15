# Early Mission1의 실제 단계 조건과 Totalmade

2026-10-15, **VM-W11-Codex**. AGENTS.md와 양 인수인계를 재독하고 재개했다. 현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 원본10.78의 `004c3a20` 전체 분기와 두 판본 Totalmade의 허용/초기화/조회/부류 집계를 복원했다. 실제 미션 화면의 자동 단계·간헐천 생성·다리 입력·수확을 연결하기 위한 공통 모듈이며, 이 단위만으로 Early Missions 완주가 가능해진 것은 아니다.

## 확인한 원본과 구현

`RawTutorialOne`은 미션 게이트와 **같은 stage 참조**, 현재 localPlayer/battleShown/명령 토큰, 같은9×0xb4 Player 표와`00595094/98`의 **최근 생성 최대5개 간헐천 목록**을 사용한다. 이 표는 AssetPostPopState의 전체 간헐천 목록과 별개다. `00486130`이 호출마다 지우고 생성SID를 쌓으므로 상위가 여러 Player를 처리하면 마지막 해당 Player의 결과가 남는다. 별도 월드나 진행 상태를 만들지 않는다. 실제 집계/Graph/SP/단계 증가/간헐천 생성은 명시 경계다. 게이트의 잠금·정지 거부를 무시하는 자체 단계 증가는 없다.

| 단계 | PATCH의 실제 조건과 순서 |
| --- | --- |
| A | 이벤트 하위26비트가 현재 F4명령과 같으면 집계 조회를 생략하고 Advance 요청. 그 외에는 다리genus4의 두 번째 로컬 집계가0보다 크면 요청. 마지막 reclaim명령 비교가 같으면1을 반환한다. |
| B | 같은 집계가7보다 클 때 Advance. |
| C | 같은 집계가18보다 클 때 Advance 뒤 `00486360` 간헐천 생성 호출. Advance가 잠금/정지로 거부되어도 생성 호출은 유지한다. |
| D | 현재 Player+7c의 **신전**SID Graph를 한 번 읽고 현재 간헐천 목록을 순회한다. Graph가 같은 모든 항목에 Advance를 요청한다.0/254를 별도 무효로 거르지 않는다. callback 뒤 count를 다시 읽는다. |
| E/F | battleShown이 비제로일 때 실제 SP float32를 읽고 각각200/600 이상이면 Advance한다. |

SP 비교는 실제 `fcomp→fnstsw→test ah,1`을 확인했다. 정밀 C의 `!(sp<threshold)`를 그대로 쓰면 NaN에서도 진행하므로 적용하지 않았다. NaN은 원본처럼 진행하지 않으며 두 임계값과 float32 바로 앞뒤를 독립 대조했다. 단계 함수는 A의 reclaim소비만1, 나머지는0을 반환한다.

`RawTotalMade`는 기존 **SquidPostPopState의 세256DWORD 집계 표를 직접 참조**한다. 현재 Pop/Unpop의 집계 결과를 다른 배열에 복사하지 않는다. 허용/금지 표는 원본처럼 별도 공유DWORD 표이며0이 허용이다. `AllowAll`/`ResetCounts`는 원본 초기화 범위인 **첫255항목**만 쓰므로256번째 물리 칸을 보존한다. 타입70부터 numTypes직전까지 genus가 한 비트라도 겹치는 두 번째 로컬 집계를 더하며 DWORD 감김 후 signed결과로 읽는다. 패치의 잘못된 getter타입/빈genus assert와 CD의 조용한0반환 차이를 유지했다.

## 근거와 독립 검사

같은 현재 PC의 정밀 프로젝트를 `tools/ghidra/export_functions.ps1 -Name tutorial-one -Refined`로 읽기 전용 내보냈다. 목록은 [tutorial-one-functions.json](../../tools/ghidra/tutorial-one-functions.json), 결과는 `extracted/tutorial-one/`다. PATCH10함수/CD9함수다. 최초 CDgetter주소 두 개를 잘못 지정해6/9가 되었으며 원문에서 실제 시작점`0040bac0/0040bb30`을 확인해9/9로 교정했다. 최초 결과/로그는 `originalCD-initial*`에 보존했다. CD의 tutorial1은 inline Advance·Player폭·SP조회가 다르므로 **RawTutorialOne은 PATCH전용**이다. CD동일 구현 완료로 세지 않는다.

REA6.5.0은 보호 PE의 형식/해시를 인식했지만 Ghidra `analyze_function`은 `H:\\`경로속성의 Win32오류87로 분석 시작에 실패했다. 같은 요청을 반복하지 않고 세션을 닫은 뒤 기존 정밀 Ghidra 내보내기와 보호 PE명령의 독립 실행을 사용했다. REA설정/설치/원본을 변경하지 않았다.

[독립 실행기](../../tools/decomp_tutorial_one.py)는 원본 PE사본을 Unicorn으로 실행한다. 선택 export의 포함 명령 범위 밖 실행과 스택/선언표 밖 쓰기를 거부한다. 정상복귀·보존레지스터·ESP·SEH·x87제어어027f를 확인한다. Tutorial의Advance/Graph/SP/간헐천생성/집계조회와debug진단은 인자·순서 관찰 경계이며 실제 원본 프로그램/장치를 실행한 것은 아니다. Totalmade집계와조회/초기화몸체·TYPEgetter는 실제 원본 명령을 실행한다.

| 고유 입력 | 수 | C++ 대조 |
| --- | ---: | --- |
| PATCH Totalmade |52| 반환·assert수·집계3개와허용표의전체각1024바이트 |
| CD Totalmade |52| 같은전체표·판본별범위동작 |
| PATCH Early1 |266| 반환·외부조회/Advance/생성의전체순서 |
| 합계 |**370**| 집중3개·실패체크0 |

추가 공유상태 검사는 같은 집계 변경의 즉시 반영과D분기중 callback의목록축소/단계변경을 확인한다. 기대결과fixture는 `cpppj/tests/fixtures/tutorial-one-x86.tsv`, 바이트식별/고유입력보고는 `cpppj/tutorial-one.json`이다. 도구의 주석교정 후 같은370입력을 재실행했으며 중복을 새고유입력으로 더하지 않았다.

같은 단위의 [공통 프레임 게이트](cpp-mission-frame-gate-20261015.md)는 실제4몸체328고유입력과집중2개로 확인했다. 두 묶음은 합계 **698고유입력**, C++집중 **5개·실패0**이다. 통합 NetstormCpp/netstorm_tests Release빌드 경고/오류0이다. 전체CTest는 **1/1통과·내부1,174개·실패체크0**(테스트121.34초/전체121.37초)이다. 이 전체실행에는당시사제메뉴3개도포함하며이후추가된사제메뉴검사는단위38에별도로기록한다. 로그는`extracted/run37-38-ctest.log`와`cpppj/build/Testing/Temporary/LastTest.log`다.

## 실제 연결에서 남은 작업

현재 UberGump는 A.즉시브리핑을 표시하고 닫을 때 즉시재개한다. 실제 게이트의 최초10프레임·닫힘7프레임·후속0.5초/10프레임과 같은 정지소유권으로 전환해야 한다. Mission중 B./C.현재섹션조회, 같은월드집계/Graph/SP접근, 실제승패/임시창정리/간헐천생성도 연결해야 한다. Tutorial2~6,생산/다리/건설/수확/운반/승패저장과실제완주동일성은 미완료다. 원본DirectDraw전체화면오류는 사용자지시대로 제외한다. dotnetpj·AGENTS.md·보호원본 변경과원격push는 없다.
