# Tell의 SOLOLIST/FORTLIST 메뉴 부모 복원

시작 HEAD는 `3b88f86a / 20261023-21 CP`다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 기존 정밀 원문과 보호 PE를 읽었으며 새 디컴파일/원본 게임 실행/기존 산출물 변경은 없다. REA 공급자의 기존 H 드라이브 admission 오류는 재시도하지 않았다.

## 원본과 범위

`extracted/refined/originals/Netstorm.c:159304`의 실제 부모는004c83b0이다. 일반 `$Menu`는 목록 이름을 판별하기 전에 현재 메뉴 수(+21c8)가0이면 BeginVertical(0x40000)을 호출하고, 복귀 후 현재 메뉴 수를 다시 읽어1을 더한다. 빈 목록도 열을 만든다.

`*SOLOLIST*`는 dirty1→같은 키 버퍼에SOLOSpec→Refresh2→현재 Row 반복→dirty1→fortSpec이다. `*FORTLIST*`는 dirty1→fortSpec→Refresh0→같은 Row 반복이고 마지막 dirty/키 복구가 없다. 대소문자를 무시하지만 `*FORTS*`라는 별칭은 없다. 성공 행 뒤 index를 증가시키고, 그때의 현재 count가15초과이며 index==count/2일 때 새 열을 만든다. Refresh의 반환 개수로 고정 for문을 만들면 콜백 변경을 놓친다.

004cb210의 실제 선택 명령은 **ReadFort=020000ca**다. 보호 명령 표00542b90에는 문자 그대로 LoadFort/EditFort가 없고, AskEditFort=020000c9, NewFort=02000079, DoNothing=02000089, Tell=02000095다.

## 변경

- `TellMapMenus`는 위 부모의 현재 메뉴 수/반복/정상 복구를 구현한다. 비맵도 공통 첫 열 효과를 수행한 뒤false다. 선택하지 않은 장치는 요구하지 않으며 예외 뒤 임의 finally 복구를 만들지 않는다.
- `MakeTellMapMenuHooks`는 실제 SoloMapCatalog/SoloMapFileSpec과 현재/예비 이름 getter를 연결한다. 기존 파일 분류/State/Subscriber/표시 몸체를 복제하지 않는다. 실제 Row 문자열과020000ca를 소비자에 전달한다.
- `DialogMetadataHooks`는 1차 HTML 뒤 실제 메타 추출 시점에 action 조회/변환 효과와 Menu/Checked 추가 장치를 제공한다. hook이 공급되면 sentinel의 placeholder 조회 효과가 목록보다 앞선다. 종류 검색 우선순위와 제거 경계는 유지한다. 기존 호출자가 장치를 생략한 동작은 유지하며 UberGump의 전체 명령 표/80칸 인자 고리까지 연결한 것으로 세지 않는다.
- 페이지와 항목은 현재 메뉴 열 수/열 번호를 보유한다. 이 단위는 파서와 파일 목록 부모의 연결이며 UberGump의 실제 표시/선택 상태 연결 완료로 확대하지 않는다.

## 독립 관측

`tools/decomp_tell_map_menus.py`는 보호 PE SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`의 **004c86fe 진입~004c8b77 정상 합류 또는004c88c5 비맵 분기**를 실제 실행한다. EBX=this, ESP+b4=label, 현재 메뉴 수/dirty/키/count는 명시 입력이다. 완전한004c83b0 호출이나 전체 Tell UI 실행이라는 뜻은 아니다.

477bb0 BeginVertical,4cafd0 Refresh,4cb210 Row,4e65ed CRT 비교만 명시 외부 경계다. Refresh는 명시 count/dirty0을 공급하고 Row는 index<count와 첫 성공 뒤의 명시 변경을 공급한다. 실제 부모 기계어가 index/현재 count 재조회·키/dirty 쓰기·메뉴 증가·분기를 수행한다. 하위 Row/Refresh 몸체는 기존 별도 원문 검사를 사용하며 이번 관측에 중복 합산하지 않는다.

864입력×x87제어어27f/37f의 **1,728관측·두 정밀도 차이0**, 실제 지정 PE 본체 **503,568명령**, 모든 종료 ESP차이0이다. 0/1/15/16/17/31/40/50개, 기존 메뉴 수, 초기 dirty/키, 대소문자·비맵과 다음 콜백 변경을 포함한다.

- 첫 Begin이 메뉴 수를5로 바꾸면 원문이 다시 읽어 증가한다.
- 첫 성공 Row가 count를32로 바꾸면 원문이 새 count로 분할/반복한다.
- 첫 성공 Row가 키/dirty를changed!/7로 바꾸면 FORT는 보존하고 SOLO만fortSpec/1로 복구한다.

원문은 새 `cpppj/tests/fixtures/tell-map-menus-x86.tsv`에 앞7열 입력/뒤6열 관측으로 보관한다. 보고서는 `extracted/cpp-tell-map-menus-20261023-native.json`이다. 기존 fixture는 수정하지 않는다.

## 검증과 남은 연결

새 검사는 독립1,728관측 전체 대조, 빈/비맵/필수 장치 누락/예외, 실제 현재Config의 숨김 처리와 메타 종류 우선순위/분할 열/행 표시 관문을 확인한다. 별도 통합 검사는 보호 Fort를 실제 파일 장치로 읽어 50개 상한·열 분할·명령/이름·현재 이름 우선 선택·TEST01 단일 목록을 확인한다. 현재 보호 디스크에는433개가 열거되므로 TEST01이 처음50개 안에 반드시 나온다고 가정하지 않는다.

최종 Release 빌드3은 **종료0·경고0·오류0**이며 앞선 빌드1/2도 경고/오류가 없다. 관련 **21검사·실패check0**(새TellMap4/Catalog9/HTML6/List2), 최종 전체 CTest1은 **2,347검사·실패check0·종료0/178.69초**다. LastTest.log의 내부 집계2347/0을 확인했다. 제품/검사 C++는 빌드3/최종 집중·전체 검사 뒤 바꾸지 않았다.

최초 통합 검사의 TEST01이 첫50개 안에 있다는 검사 가정만 실패했다. 원본433개 열거/50개 상한을 보존하고, 전체 순서는 캐시 원문과 비교하며 TEST01은 별도 실제 단일 경로로 확인하도록 수정했다. 제품 목록이나 독립 원본 기대는 변경하지 않았다. 숨김 메타 검사는 실제 Config 조건 hook을 명시 연결한 상태로 최종 검사했다.

모델 용량 오류 후 현재 변경 파일을 다시 점검했다. 잘린 코드/누락된 빌드 등록은 없고 미완료였던 검증 문구와 인수인계를 마감했다. 그 사이 별도dotnet 작업은 `11dd2898 / 20261023-22 DN`에 이미 커밋되었으며 이번 CP 커밋에는 포함하지 않는다. UTF-8·추가 한국어 주석·diff/보호 경로 미변경을 확인하고 **20261023-23 CP**로 로컬 커밋한다. 로그는 `extracted/cpp-tell-map-menus-20261023-*`에 있다.

다음은 UberGump의 현재 사용자 이름/키/파일 캐시 수명, 다열 표시, AskEditFort/ReadFort의 mode/pending/다음State 요청이다. Editor의 첫 객체 생성 전 공유 모드·generic Fort 설치·현재 월드 snapshot/저장·새 파일/입력·기술 변환/Deck/Editor Player 초기화는 여전히 필요하다. 기존 캠페인 Load에 editor 플래그만 더해서 대신하지 않는다. 상세 의존성은 직전 문서와 후속 인수인계에 유지한다.

후속 읽기 전용 감사에서 확인한 실제 연결 계약은 다음과 같다. 보호 PE의 명령 인자/호출 순서도 함께 확인했으며 새 디컴파일은 하지 않았다.

- 사용자 이름00540ca8의 PE 초기 포인터는00540c78이고 첫BYTE0이다. 현재 Client 수명 이름 저장소는 없고 RawCampaignSession::Impl의64BYTE handle이 캠페인 부모에서 번역You로 바뀐다. 기존Fort 준비4b5ca0은Subscriber, 새Fort4b3920은Unnamed, Editor004904b0은Player로 같은 이름을 갱신한다. 이름을 임의userName 설정 키로 읽으면 안 된다. Session 해제 뒤에도 현재 이름을 보존할 Client 연결이 필요하다.
- 입력 부모004d62b0은 먼저Guide→Gump에 전체 사건을 전달한다. 둘이 소비하지 않으면 release80000000을 거부한 뒤 code&03ffffff로 일반 명령을 비교한다. Control/Shift를 별도 금지하지 않는다.
- AskEditFort(c9), PE004d7408: mode005cae84=2→PathSpec(lastEditFort,0,0,0)→pending00460bb0→Refresh2→Tell LoadEditable다. dirty/current/제목/State 요청은 이 본문에 없다.
- ReadFort(ca), PE004d745f: **event.y** 문자열→pending00460bb0→현재mode 재조회→0이면004b3550(next6), 그 외004b3560(next1)이다. 즉시 월드를 설치하거나 제목을 바꾸지 않는다.
- pending00460bb0과 current00460980은 다른 경계다. 후자는 snapshot/manager 이름/이전 파일 삭제 요청/제목/dirty/조건부lastEditFort까지 동반하므로 선택 명령에서 대신 호출하면 안 된다.
- 004b3560은 기존 pending/next 여부를 검사하지 않고 매 호출nextState=1을 덮어쓴다. `State::Post`의 이미 대기 중이면 무시하는 동작으로 대신하면 안 된다. inBattleMode이고currentState14일 때만 추가로현재Fort 경로→004bf550→State flags&~2→현재게임시각 정수 변환을State+c에 기록→현재owner 섬 수<=0이면flags&~4/Notify다. 일반 메뉴에서는 nextState 쓰기만 수행한다.

Editor 초기 진입은 원본004b6dd0의 State1/2가 editor/missionPost/fullDeck/inFortMode를 첫 객체 생성 전에 함께 설정한다. 기존Fort는alternate0, 새Fort는004b5ca0이alternate1을 써서 Object/Money/Badges/Technology child를 생략한다. 카메라004340d0/생산UI0047d890은 기존 실제 모듈을 재사용한다. CoreData 읽기004beca0·쓰기004beb30과004923e0은Editor 가드로 자연 생략되지만 **004bf9a0의 현재 월드 전체 snapshot은 필수**다. RawFortImage는 기존 섹션 묶음이며 이 live snapshot을 대신하지 않는다.

필수 snapshot child는TypeNames004bd6f0, 현재기술004bcea0, 실제객체004be430, Territory120BYTE004bf6d0, Deck004befd0, SP004832c0, Mission00463a80, Badges004be7f0, Notify004bd360이다. 현재 이 writer 부모는 미복원이다. Editor Player2..8의004904b0도 모든타입중group!=10&&theme!=5에 직접ContainedCreate(parent,type,0,4,0x10,0)을 호출하므로 Ensure/AddList로 대체하면 안 된다. 관련 원문 시작은145526/152778/115716이다.

전체 Early Missions·캠페인·데모3개 사용자 완주, Edit/커스텀 시나리오·메인 메뉴와 GUI/음향 동일성·배포 목표는 계속 활성이다. 원본/CD·AGENTS와 다른 프로젝트 수정, 원격 push는 없다.
