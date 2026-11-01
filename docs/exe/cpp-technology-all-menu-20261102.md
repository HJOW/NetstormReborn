# HTML 전체 기술 목록과 제단 메뉴 부착 — 단위706

2026-11-02, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의 2026-10-23 자료다. 705의 원소별 목록에 이어 전체 *TechList*004cb690을 실제 HTML/Session/메뉴에 연결했다. **기술 선택 후 요청 전송은 아직 미연결**이다.

## 구현

`TechnologyListMenu::AppendAll`은 대상 DWORD를 WORD로 접고 가상+80의 초기 레벨을 읽는다. 기존 `MenuGumpWidgets::Reference`004644e0으로 현재 controlled 메뉴를 같은 공용 SID 풀의 실제 대상 Form에 연결한다.

원소0..3마다 사용되지 않는 후보/보유 NULL 수집의 조회·진단도 보존한다. 번역된 원소 이름→PE의 실제 `~y%s` 머리→DoNothing 비활성 행→기존 원소별 목록→구분선 순서다. 원소는 Wind/Rain/Thunder/Sun 순서다. 마지막 보유 수/전체 수 집계는 원소·레벨 필터 없이 사제와 group10만 제외한다. 두 번째 현재 기술 부모 조회도 유지한다.

표면은 실제 float 좌표에 PE00500edc의 float 0.99989998340606689453125를 확장해 더하고 0쪽으로 절삭한 뒤 같은 hash0 WORD 표에서 읽는다. 범위 밖은0이다. 업그레이드는 물리 범위 안 표면의 **004acda0 섬 번호 WORD와 Player+78 본섬 번호**를 비교한다. Player 주소 선택→섬 getter→같은 Player 필드 읽기 순서를 보존했다. 그래프 BYTE와 구별한다.

최대 레벨은 `Altar at Max %sLevel`에 원문00503c8b의 빈 문자열을 넣은 비활성 DoNothing 행이다. 나머지는 초기 가상 레벨+1의 표시값과 UpgradeAltar020000d4/대상 WORD를 쓴다. 마지막 행은 `Get 5,000 ~[IfortGump.3]`/GetNewTech020000d5/nugget TYPE다. 즉시 기술 지급0200009f로 바꾸지 않았다.

전체 보유 시 랭크 분기는 정적으로 작성했다. 0048f0d0의 같은 현재 Core와 noIsland TYPE+174/+178, 즉 B/C 프레임 개수 곱-1을 사용하며 반복 getter 순서를 보존한다. **이 분기의 새 단위/GUI/통신 검사는0**이며 멀티플레이 검증 완료로 세지 않는다. 원문 heap 할당 실패·SEH·손상 포인터·전체 ABI 동일성은 주장하지 않는다.

## 검증

[원문 도구](../../tools/decomp_technology_all_menu.py)는 실제004cb690·004cb320·004cb3f0·004cb4b0과 CRT004e49c0을 **100입력**으로 실행했다. 레벨-1..4, TYPE16/80, 부분 보유/미보유, 본섬/다른 섬, 음수/255/256 좌표, 표면0/1, 섬 getter 뒤 Player 변경을 교차했다. 전체 보유 랭크는 도달하지 않는다.

메뉴/Form·TYPE 주소·현재 기술/소유 종류·섬 getter·번역/설명/printf·행/구분선·heap/진단은 명시 경계다. 부모 제어와 실제 좌표 CRT는 대체하지 않았다. [fixture](../../cpppj/tests/fixtures/technology-all-menu-x86.tsv)와 [원문 보고서](../../cpppj/technology-all-menu-native-20261102.json)를 따른다. 부모의 긴 경로가 기존0.5초 한도로 중단되어, 이 도구에만3초/100만 명령의 유한 한도를 주었다. 100입력 모두 정상 반환·스택 복구·목록8할당을 확인했다. 기존 fixture는 덮지 않았다.

새 C++2개는100행의 전체 호출 순서·행 문구/명령/인자/활성 상태와 실제 HtmlWindowMetadata의 전체 목록→같은 작성기/현재 열이다. 기존705의3개와 HTML 메타1개를 포함해 **6개 통과·실패0**이다. 기존367 원문과 HTML128행은 재생이며 새 원문 수에 합산하지 않는다.

Release `build-v1` 제품/검사와 `build-v2` 집중 검사 재빌드의 경고·오류는0이다. 최초 `focused-v1`은 HTML 준비 노드의 미초기화 menuColumns 때문에1단언이 실패했다. 두 HTML 준비 노드를 실제 생성 직후처럼 값 초기화한 뒤 `focused-v2`6개 실패0이다. 원문100 대조는 최초부터 통과했고 제품 수정은 없었다.

## 제품과 인계

실행 제품706/표시11.01 SHA-256은 `4b2d1b9151caff8e57bc682e4eaf99746809dd530582422168d03c326f8fd2e1`다. [제품 보고서](../../cpppj/technology-all-menu-20261102.json)와 원시 `extracted/cpp-technology-all-menu-706/`를 따른다. 기존 AGENTS·원본1394·CD425·fixture804의 **2624개 변경0**, 새 fixture1 포함 현재2625개다. AGENTS/전체 디컴파일은 수정하지 않았다.

이번 게임/GUI 실행은0이며 최신 실제 GUI는704 제품이다. 전체 C++/전체34/새 ZIP, 자연 완주·전체 픽셀 동일성·PCM/물리 입력, 포획/희생·특정 적 공격·승리·공중3종 전용 행동·MP/통신 검사는 하지 않았다. 전체 목표는 미완료다.

다음은 GetNewTech/RequestStormpower/UpgradeAltar의004d82ed/004d8351/004d83b7→0040af50→004e2dc0 요청 경로다. 0040af50의 헤더 BYTE는 PE0052ee65&7f=1이다. 현재005453dc는 기존 `CarrierInputState::dais`와 같은 저장소를 사용해야 한다. `RawPriestSacrificeRequest`는 희생 시작 wrapper여서 보상 선택을 대신하지 않는다. 즉시 기술 지급으로 합성하지 않고 프로토콜/서버/로컬 부모를 확인한다. 새 MP 검사는 하지 않는다. HTML Input004d3ed0/Zsend도 남는다.
