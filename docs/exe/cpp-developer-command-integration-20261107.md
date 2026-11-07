# 1035 — 개발자 문자열 수신과 핵심 명령

10.78의31h/0044dba0→0040a6f0(packet,0)→0044dc70과40a6b0 길이 observer를 실제 Login/일반·단독·Editor Session에 연결했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 정밀 C와 보호 PE를 정적으로 대조했으며 원본 실행/수정과 신규 MP 검사는 하지 않았다. **31h 전체 해석기 완료는 아니며 핵심16개를 접합했고 나머지17개는 미완료다.**

31h observer는 수신에서 자연 반환하고 송신에서만 현재len DWORD가-1이면 _p_packets.cpp74b 진단을 부른 뒤 현재lowWORD+8을 헤더WORD에 감겨 쓴다. receiver는 sender/len DWORD를 해석하지 않고+8의첫 NUL 문자열을 실제 해석기에 넘긴다. 버전1/정책0/가변 길이이며 실제 callback pin/기록/생존 관문은 기존 분배기를 유지했다. 새31h writer나 임의 전파는 만들지 않았다.

32h의 같은 현재7DWORD를 직접 쓰는 수신 overload를 추가해 Login 밖의 일반/단독/Editor에도 등록했다. 원문처럼 입력0..6을현재 bytes에서 차례로 읽고 먼저 쓴 값을 되돌리지 않는다. Editor 등록/반납 목록은41개이며31h/32h를 같은 Session 수명에 포함한다. 이전1034의 비Login/Editor32h 연결 잔여를 해소했다.

연결한 명령16개는 `sethackmove, setnodamage, shootmyfriends, fillmytank, generatortimer, quickvortex, quickbuild, roundtotile, combatGumpColsRows, setally, clrally, destroy, create, scatter, setabstract, applydamage`이며 각 원문 별칭도 처리한다. 설정은1034의같은 현재7DWORD를 사용한다. 미기입 입력의 원문 stack 값을0으로 합성하지 않고, 원문이 기본값을 정한shootMyFriends/quickbuild는1, cr은3/10을 사용한다. fillMyTank는 정수 저장만 하며 도움말 설명으로 에너지 변경을 추가하지 않는다. roundtotile/rtt는 원문처럼 인자를 읽지 않는 반환이다.

quickBuild/quickVortex는 PE의0055a49c/0055a498 BSS0을 Client GameAux에 두고 기존 건설 Run의 비영 관문에 bind했다. cr은cols1..3/rows1..16 제한 뒤 곱이8보다 크면GeneratorTimer0을 쓰고 현재0054288c(최초02000065)와두값을 실제Client Input.PushCommand에 넣는다. **해당65h UI 소비자의 전체 접합은 후속 확인 대상이다.**

create는 실제factory→직접raw+36 frame DWORD→고정Pop20/20이다. scatter는x=3..30/간격3의10개를 만들고 타입의실제프레임 개수/같은Client15비트 CRT를 사용한다. GameRandom으로 대체하지 않는다. 현재5326d8 행의PE 최초값은3이며 마지막에signed(현재행+2)%10으로 갱신한다. Rand 호출을 나눗셈 검사 앞으로 보완해 실패 시의 선행 CRT 상태도 보존했다. setabstract는Unpop0→isVoid 진단→인자lowBYTE bit0만raw+40 갱신→Repop0 순서다. applydamage는 기존 정기 피해의 실제 가상98 분배 callback을 공유하고 소유자/공격자0을 넘긴다. destroy는server/parse/free 관문 뒤 같은 파생 삭제를flags0으로 호출한다.

PATCH PE에서clrally/setally는 실제같은48f6a0(AddAlly)으로 합류한다. 이름을 이유로ClearAlly로 바꾸지 않았다. PE를 추가 확인한setlocalhitpoints는 제공된HP를 사용하고setowner는 같은 지역의signed lowBYTE를 사용한다. 이 둘은 아래 미접합 목록이며 Ghidra C의오합된지역 변수/float표기를 후속구현에 그대로 옮기지 않는다. 4345e0은 네트워크 동기화/trace가 아니라 좌표 투영과3겹사각형/즉시present 화면 강조임을 확인했다.

남은17개는 `help, setsubid, setbridgekillperiod, dump, setowner, dumptypes, dumptype, setlocalhitpoints, settracelevel, heaptrace, dumpArtData, dumpplayers, dumpgraph, dumpally, queryDirectDraw, addLabel, getPriest`다. 현재는 이 알려진 명령을unknown의자연반환으로 숨기지 않고 명시 오류를 낸다. 전체31h 기능/안정성 완료로 세지 않는다. 원문help의dt와달리 실제dumptype도dts를 검사하므로 앞의dumptypes가dts를 먼저 소비하는 점도 후속에 보존한다. 현재 순수 unknown은 원문처럼 추가효과 없이 반환한다.

CD판31h는4a4270→40a2d0→4a4290이며observer40a290의진단줄은665다. 정밀marker의자동 대응보다 실제caller를 기준으로 기록했다. 이번 실제해석기와slot frame/현재 전역은PATCH 전용이며 CD전체복원 완료로 세지 않는다. 손상된입력의미정stack/범위밖접근·잘못된추가sscanf포인터로인한스택부패 자체는호스트에서합성하지않는다.

첫/보완 Release 제품 빌드 **각각종료0·경고/오류0**. EXE **12,087,296바이트**, SHA-256 **`a8bc7ebcb918c6988969069999edf876b68566f2406c6540f37fa47ca1472c8e`**, 표시 버전11.01이다. AGENTS/원본/CD/기존fixture **2800파일 불변**, 신규fixture0이다. 신규 MP 단위/통신/게임 및 지정된 사용자 확인·공중3종 전용 검사는 실행하지 않았다. 실제 LAN/명령/게임 동작 검증 결과가 아니다.

다음은31h 나머지17명령/화면 강조·진단·입력과cr65h 소비자다. 나머지 수신/전송·재동기화/Activate·메타/Outpost/checking5 뒤 한국어 후속→시나리오UI 순서를 유지한다.
