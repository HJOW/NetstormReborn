# State1 현재 Guide의 Fort 요청 — 단위713

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**이며 정밀 자료 기준일은 **2026-10-23**이다. 10.78의 `004b50b0`·`004b32f0`·`00463b30`과 기존 State1 외곽을 대조했다. CD 10.72의 새 몸체/게임 대조는 하지 않았다.

## 변경

기존 State1은 파일 설치 뒤 현재 Mission이 비영이면 범위 밖 예외로 끝났다. 원문은 이 위치에서 `004b50b0`을 호출하고 편집 World/Deck/사제/카메라/저장/HUD 및 공통 phase 확정 전에 반환한다. `InstallExistingEditorFort`와 실제 Session checkpoint가 이 조기 반환을 구별하고, Uber의 같은 State1 분배기가 현재 Guide의 지연 State7 부모에 인계하도록 연결했다. 편집 loaded와 UserInput을 먼저 공개하지 않는다. 미션 장치를 공급하지 않은 독립 한정 장치는 기존 outside 진단을 유지한다.

`004b32f0`은 현재 Template manager와 fort 모드가 비영일 때만 동작한다. 선택 이름이 있으면 같은 handle의 NUL까지 복사→`fort.mySubHandle` 설정→제목 갱신을 수행하고, My에 고정 인자 ID를 쓴 다음 현재 owner/handle로 실제 Subscriber 노드 writer와 Notify를 호출한다. 이름 부재는 설정/제목을 건너뛰며 큰 효과 뒤 현재값을 재조회한다. Session은 기존 `SaveFortSubscriber`/등록/typed stream/공유 Client My를 그대로 사용한다. 별도 가짜 Subscriber 노드나 Player 표시 이름을 만들지 않는다.

새 Guide 요청은 `(1,"Champion")` 변경 뒤 현재 handle의 한 슬롯만 작성한다. 일반 Init `004b51e0`의 Fort 재선택/네 정적 이름과 구별한다. Zone 표식은 `0x82000004`, 전체512BYTE는 먼저0xff, 관찰 전 선언 길이는4이다. 기존 관찰/복제에서86BYTE를 소유하고 같은144BYTE next의 주소DWORD2+BYTE1·상태7·현재 포트·복제 핸들만 바꾼다. 복제 뒤 현재 순번을 증가하며 나머지 stale 필드를 보존한다.

호스트에는 이미 소비된 이전 요청 owner가 남을 수 있으므로 새 요청 성공 뒤 교체한다. 미소비 이미지는 덮어쓰지 않는다. 새 요청은 같은 Guide/Config/작업 파일 namespace와 기존 State7 부모를 사용하며 이름 선택을 요청 단계에서 반복하지 않는다. State7 이후의 기존 파일 reader는 유지한다.

## 근거와 검사

- 제품 작성 전에 [원문 도구](../../tools/decomp_guide_fort_request.py)로 **요청144+Subscriber54=198입력, 실제72627명령**을 고정했다. [최초 fixture](../../cpppj/tests/fixtures/guide-fort-request-x86.tsv)의 SHA는 `3e1e2a6ed1a96cef111d3ba837adcdcbacf3ceddc1fbed473dacbe75ad3443f9`다.
- 실제 `004b50b0/4b32f0/463b30` 및 기존 패킷/슬롯/관찰/복제 몸체를 실행했다. Config/제목/Template writer·Notify/힙은 명시 경계다. manager/fort 자연 관문·이름 부재·서로 다른 stale 바이트·현재 owner/handle/주소/포트/순번 변경·감김·전체 요청과 양쪽8BYTE·64BYTE handle·전체512/복제86BYTE·호출 순서를 대조한다. 정상 스택과 x87 제어어도 확인한다.
- **새5/기존46 집중51개 실패0.** 새 검사는 원문 요청144, 원문 Subscriber54, 부분 실패/단락 평가, State1 조기 반환, 실제 Session의 파일→Champion→typed Subscriber 노드/동일 Client My→State7 요청과 편집 공개 거부다. 기존 요청2·외곽2·기존 checkpoint42도 통과했다. 기존 외곽의258행 중148행 대조/110행 제외를 새198행 전체 대조와 구별한다.
- 최초 Native/빌드/검사 실패는0이다. v1 빌드 뒤 수명 검토로 소비된 이전 요청의 guard를 보완했고 **v2 최종 빌드 경고/오류0**, 최종 요청/실제 Session2개 재검사도 통과했다. 검사 코드의 버퍼 조회는 실제16BYTE MemStream의 data/end와 buffer 장치를 사용한다.
- AGENTS·원본1394·CD425·기존 fixture810의 **2630개 전체 변경0**이다. 새 fixture1개를 더한 다음 보호 기준은 **2631개**다. AGENTS는5867f35e 그대로다.

## 한계와 다음

실제 Uber의 수명 guard/호스트 전환은 컴파일과 정적 연결까지 확인했다. 새 게임/GUI/물리 입력/전체 State1→7→14 화면 연쇄는0이다. 실제 Session 검사는 파일 설치와 요청 작성·공개 거부까지이며 접속 성공을 합성하지 않았다. 원문 외부 경계의 전체 Config/32비트 FILE·SEH ABI/손상·할당 실패는 별도다. 기존 자연 미션 완주나 전체 동일성을 새로 검증한 자료가 아니다.

남은 State3 제목 변경 후 공통 Fort 분기와 State1/7의 나머지 조건, 지형/객체/시나리오 writer·파일 수명 및 연속 Retry/Next/Abort를 이어 간다. 이 단위의 단독 Guide Zone 검사는 TCP/멀티플레이/Outpust 검사가 아니다. 신규 MP/통신 검사, 제외된 포획/희생/특정 적 공격/승리와 공중3종 전용 검사, 전체34/전체 C++/CTest/ZIP·PCM은0이다. 마지막 실제 GUI는704, 전체34/ZIP은645의644제품을 유지한다.

최종 제품은 **713 / 표시11.01**, EXE SHA-256은 `6122593ad2dd110584d74636dd525c7d39d928792732fe23e4f6f64ba89e7d7d`다. [완료 보고서](../../cpppj/guide-fort-request-20261102.json)와 [독립 원문 보고서](../../cpppj/guide-fort-request-native-20261102.json)를 따른다. 원시 로그/보호 장부는 `extracted/cpp-guide-fort-request-713/`에 있다. 전체 goal은 계속 미완료다.
