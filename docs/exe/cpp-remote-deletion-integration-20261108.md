# 1171 — M6-d 원격 삭제 통지와 실제 공통 삭제 부모

2026-11-08 / HJOW-Athlon. 이전CPP1170/77a0c3a4, 제품11.01이다. 같은 호스트의 Client 정밀2026-11-03·R2026-11-07 C/PE를 읽었으며 생성 디컴파일 소스는 바꾸지 않았다. 제품 컴파일·허용 비멀티 집중 확인의 최종 값은 [보고서](../../cpppj/remote-deletion-integration-20261108.json)에 기록한다.

53h는 packet+4 WORD의SID1 이상/현재 capacity 미만과 현재free만 확인하고 같은 실제 World.Destroy(sid,0)을 호출한다. 62h는 signed count0 이하면 끝나며 각WORD/현재free→삭제→같은packet의현재count 재조회 순서다. 전체목록 사본·중복제거·서버/소유자/TYPE/sender 선관문을 넣지 않았다. 62h observer는 sending0에서payload를 읽지 않고 DWORDffffffff 진단0x9ac 뒤 현재LOWWORD로길이WORD를 쓴다. 같은 Session 등록/반납과Editor45종에 연결했다. 활성 writer가 발견되지 않은53/62의 새 송신자나 UI를 발명하지 않았다.

실제 PATCH Client만 장기 Storage에서 공통 삭제 진단을 선택한다. 공통 본문은 dead선기록→현재free/권한의 정상복귀단언→현재TYPE이름 로그→현재가상Pre→종속현재목록/next저장/Release/Destroy→현재root/전파/선택해제/Unpop→현재Post→같은Pool반납 순서다. 독립 typed/CD의 기존선검사 계약과 기대자료는 유지했다. 진단과로그의callable을 선택시값으로보유하여그안에서장치를교체해도현재호출은살아있다.

가상 선택은 원문83자산(공통+10 82개·Bridge 재정의1개)와네Form VT를 사용한다. 미지/NULL VT를 공통성공으로 넘기지 않는다. 프로세스 outer/내장data VT는Pool Form VT와별개다. 실제Host의CreateOrTakeForm→AttachBaseProcess는512098 Form에TYPE/PID를쓰고Host가현재PID의실제프로세스가상함수를선택하므로파생프로세스표를Form허용목록에섞지않았다.

실제 PATCH의공통Lifecycle선검사에있던추가reward.Plan/가상Count를제거하고선택된실제효과만수행한다. TYPE flags/분류는같은현재span을읽되Post의비용은원문0059ab50에대응하는최초GameAssets 비용표다. 목록은signed count/현재저장소를순차압축하고inactive꼬리를지우지않는다. Release는void/free진단정상복귀→이력→현재TYPE보존/슬롯지우기→현재server/SID FIFO/freeCount→free/dead와tail동일단언을복원했다.

Outpost509210의고유Pre455840은별도M6-k다. 전체개별파생의모든손상입력/임의native포인터/Pool양수Reset 재진입까지완료라하지않는다. 물리배열/TYPE밖·미연결가상호스트·손상종속순환은실제선택위치에서오류이며앞선효과를보존한다. 원격수신을실제로주입하는MP/패킷/단위/통신검사는사용자지시에따라생략했다.

첫제품빌드는종료0/오류0/지역변수숨김경고1건이었다. 종료후반복변수이름만type→formType로고쳐최종두종을다시빌드했고종료0·경고오류0이다. 검증하네스를옮기면서ROOT깊이를잘못둔import실패1회는게임생성전에끝났으며정확한저장소경로로교정했다. 이실패도원시에보존했다.

최종같은EXE로별도격리1920×1080 Tutorial1에진입해정상생산/포인터legal확인/좌클릭으로다리를놓았다. 실제3eh수신0→1, 새서버SID15003/TYPE82/owner1/좌표63,49를확인하고추가90프레임대기/실제PNG확인뒤MCP정상종료0이다. 전후EXE/격리설치자산/보호2800파일은모두같다. 이는공통삭제를사용하는미리보기/정상정리의비MP회귀이며새원격53/62실행검증으로합산하지않는다.

원문/사본은 extracted/cpp-m6-deletion-{prep,d1-patch,d2-patch,d3-patch,series-patch}/, 적용·두빌드·허용비멀티실행원시는 extracted/cpp-resume-1171/다. 코드접합완료와실제동작검증범위를구별한다.

M6-d 뒤에는현재TYPE기술동기화M6-e와실제런타임수명기반, 이어f~n/M7/M8이남는다. 전체목표는미완료다.
