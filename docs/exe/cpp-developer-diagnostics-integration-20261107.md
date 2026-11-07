# 1036 — 개발자 화면 강조와 진단 명령, 일시 정지 인계

사용자가 현재 작업의 문서 반영/git 커밋까지 완료한 뒤 목표 일시 정지를 명시했다. 이번1036 단위만 마감하며 전체 목표 완료로 표시하지 않는다. 재개는 사용자의 새 지시 뒤 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md/현재git 상태 확인부터 한다. **다음 구현 순서는31h 잔여11명령/입력·cr65h 소비자→나머지MP/재동기화·Activate/메타·Outpost/checking5→한국어후속→Edit 시나리오UI**다.

이번에는10.78의4345e0 화면 강조를 실제 Renderer/Screen에 연결하고31h의 `help, dump, setowner, setlocalhitpoints, dumpplayers, dumpally`6명령을 추가했다. 앞선16개와합쳐22개이며 **나머지11개는 미완료**다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 정밀 C와 보호 PE를 정적으로 대조했으며 원본 실행/수정과 신규 MP 검사는 하지 않았다.

강조는같은497220/ProjectCameraView로 투영하고 현재snapshot+8의cellPixels[0]을양축에 사용한다. left/top은(투영−가로칸+1),right/bottom은(투영−1)이며 signed DWORD감김을 보존한다. 작은clip을쓴뒤 세 번 노랑0059afd8/basic[5] 사각형을 그리고 매번각변을한픽셀 줄인다. 4a2270의임시전체viewport/lock/네선/이전clip복구 장치를 기존DrawWater와공유했다. 끝은 기존Screen의private4a46d0/PresentPaletteClear를호출하며 장면 재그리기/Frame/난수/flip을진행하지 않는다. 원문처럼 강조의작은clip은끝에도남는다. 현재file팔레트의노랑을매stroke 읽는다.

기존Screen friend인 ClientRendererFrame이실제장치/private출력을소유하고Client wrapper→Session서비스로 연결한다. 별도모의표시나새Frame을만들지 않는다. 기존DrawWater도동일사각형장치를사용하며 현재비활성flag의선행반환은보존했다. SID0 강조는Pool/Screen을조회하지않는다.

dump는기본SID0/현재좌표강조→기존Client.TraceObject(44dab0) 전체재귀를사용한다. setowner는서버에서Unpop0→signed lowBYTE의파생SetOwner→Repop8,client에서는강조/원문로그만 수행하며 별도송신을추가하지 않았다. setlocalhitpoints는두번째입력HP를직접저장하고Repop0→강조→현재HP/최대HP로그 순서다. NativePE는입력HP를사용하며Ghidra의오합된지역변수로SID를HP로쓰는표기는따르지 않았다.

dumpplayers는현재Player1..8의flags&22만표시하고bit1로로그인접두를선택한다. 원문508780 문자열은공백없는Not이므로NotLogged in 표기를보존했다. dumpally는현재owner이름을로그한뒤48f790과같은현재+54mask를각대상1..8에서다시읽는다. 로그callback 뒤이름/mask변경도다음조회에반영한다. help는원문32개문자열을같은순서/수준29로출력한다. 이전include70h/31h 설명의위치도각줄로정리했다.

남은11개는 `setsubid, setbridgekillperiod, dumptypes, dumptype, settracelevel, heaptrace, dumpArtData, dumpgraph, queryDirectDraw, addLabel, getPriest`다. 현재알려진미접합분기는명시오류로남아있으며전체31h기능완료가아니다. heaptrace54dab0은해석기뿐아니라NativeWinMain438dc0/4395df 두프레임에서소비됨을새로확인했다. 재개시setter만추가하지말고실제heap부모도연결한다. cr65h UI소비자와실제31h입력접합도미완료다.

CD참조는화면강조4a9eb0/객체trace4a40c0이며이번실제접합은PATCH전용이다. CD전역/프레임배치·전체복원완료로세지않는다. 미정stack/손상입력의범위밖호스트접근자체는합성하지않는다.

Release 제품 빌드 **종료0·경고/오류0**. EXE **12,097,536바이트**, SHA-256 **`eed4e831db62f83d8801cf9ea3455e43e540eba8a8d23afb69c12e306c413091`**, 표시 버전11.01이다. AGENTS/원본/CD/기존fixture **2800파일 불변**, 신규fixture0이다. 신규 MP 단위/통신/게임/진단GUI 및 사용자확인·공중3종전용검사는실행하지 않았다. 실제 LAN/명령/표시 동작 검증 결과가 아니다.

이 문서와보고서·LEFT/README 반영후현재단위를로컬커밋하고goal을paused로전환한다. 사용자재개지시전에는다음단위/추가실행을진행하지 않는다. 최신전체34/ZIP/GUI/비MP검사는LEFT0.1의과거기준을유지한다. 현재제품의추가배포검증완료로확대하지 않는다.
