# C++ Fort 저장 전역의 장기 소유권

2026-11-08 / 단위1183 / HJOW-Athlon / 제품11.01.1182 뒤 기존4파일의 m2b1 준비본을 현행에 접합했다. NoWorld 전체 Snapshot·JoinTestBattle·M6-m/n은 미완료다.

객체 저장/로더의 scope005c89b0과 두 mask00542644/0054263c를 기존 FortSerializationGlobals에 보관한다. 실제 Session의 FortObjectLoadState는 version/depth와 함께 같은5DWORD 주소를 빌린다. 두 기존 typed 생성자는 자기 scope/mask 기본값을 유지하며 복사·이동 금지도 보존한다. 참조의 전체 owner가 상태보다 오래 살고, Session 생성/반납이 이 전역을 다시 초기화하지 않는다.

무상 기술 타입005412bc=153과 기본 신전 제품0054126c=133은 같은 장기 GameAux로 옮겼다. 기존 Session의 Deck/생산/AI/금지표 소비자는 해당 필드를 참조한다. getter를 추가했으며 원문에 없는 설정 writer나 입력·토글·새 World를 만들지 않았다.1182의 자원 사건 DWORD와 앞선 packet 헤더 등 현행 필드는 보존했다.

저장된 최소 패치의 RawGameAux 문맥은1174 이후 헤더와 달라 수동 병합했다. 원시5주소 전수 C 참조27함수의 완전 C/function table/실제 inclusive PE 및 실제 초기 상수는 보고서 nativeEvidence에 연결했다. 마지막 정밀 디컴파일은 Client2026-11-03/R2026-11-07의 같은 HJOW-Athlon이며 생성 원문은 변경하지 않았다.

NetstormCpp/NetstormRoot와 기존 집중 검사 대상 Release/x64가 최종 종료0·경고오류0이다. 멀티플레이어와 무관한 기존 FortObjectLoad_5검사는 모두 통과/실패0이다. Patch/CD replay, 실제 생성/소유자/HP/Pop/내용물, 누락 장치/다른 Pool 거부, 공유 FullDeck 현재성의 기존 기대값을 변경하지 않았다. 새 MP/패킷/통신 검사를 실행하지 않았다.

최종 EXE의 격리800×600 Edit→실제 TEST01→Main Menu/No→메인 복귀와 정상 종료0을 확인했다. 기존 제목 하네스의 명시 d/title950 경로에는 실행 전에 실제 TEST01.fort/english 사본을 준비했다. Test Battle·포획/희생·특정 공격·승리·공중3종 검사는 하지 않았다. 설치 사본·EXE·고정4소스·보호2800파일이 불변이다. 전체 자연 완주나 NoWorld 전체 저장 통과로 확대하지 않는다.

AGENTS.md의 새9차34버그 목록(b5197d48)이 기존 인계 지도에 없어서, LEFT_JOBS에 S4 뒤 B1~B4로 계획만 반영했다. AGENTS.md는 수정하지 않았다. B1은 전수 분류/현행 대조, B2는 공통·비MP 수정, B3는 MP 관련 정적 복원/컴파일, B4는 허용 검증/배포 마감이다. MP/직접 플레이 제외는 그대로 적용한다.9차 구현을 이번 완료에 포함하지 않는다.

다음은 같은 현재 저장/가상 조회를 사용할 NoWorld HP·보상·SP와 자원 삭제다. m1/m2a/m2c1/m2c2 준비본은 여전히 제품 미적용이고 전체 JoinTestBattle caller·Snapshot은 남는다. Source/listener/공통표/Poll·25태그 전scope/63잔여F·Node 큐/지연 큐도 미완료다. 큐는 비활성이다.

[보고서](../../cpppj/fort-save-global-owner-20261108.json)·[현재 인계](../../LEFT_JOBS.md).
