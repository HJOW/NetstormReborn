# 10차 알려진 버그 NB01~NB34 판정 현황 (제품 1285~, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음). 원문 증상은 [등록 목록](cpp-known-bugs-plan-20261109.md)을 따른다. 판정은 `수정 완료 / 현행에 이미 반영 / 해당 없음 / 미완료(조사 중)`이며, 허용 검증 결과와 사용자 확인 잔여를 따로 적는다. 포획/희생·특정 공격·승리 재현과 MP 실행은 하지 않는다.

| ID | 판정 | 근거·조치 | 검증 |
| --- | --- | --- | --- |
| NB02 | **수정 완료** | `StyleTextMarkup::Option('E')`가 오프셋 범위(-4..4) 검사를 `||`로 해서 **한 좌표만 범위 안이면 다른 좌표를 임의로 크게** 적용할 수 있었다(글자를 뒤로 밀어내는 악용). 새 `StyleTextOffsetAllowed(x,y)`로 두 좌표가 모두 범위 안일 때만 적용한다. | 단위 `KnownBug_NB02_EmbossOffsetRequiresBothCoordinatesInRange`(`netstorm_known_bug_tests`) 통과 |
| NB05 | 현행 미재현 | Test Battle 채팅 줄에 936자를 입력해 전송했으나 입력줄이 상한(약 87자)에서 멈추고 메시지가 정상 전송되며 종료/로그 없음. | 실제 실행(`extracted/cpp-resume-1278/nb05/`). 사용자 확인 잔여: 존/MP 채팅의 긴 입력 |
| NB07 | 현행에 이미 반영 | `Create New Map`에 기존 이름을 넣으면 `Unique Name Required — A Fortress with that name already exists...` 안내가 뜨고 기존 fort를 열지 않는다. | 실제 화면(`extracted/cpp-resume-1278/nb07/`) |
| NB08 | **수정 완료** | 세 겹의 원인을 실제 실행으로 찾아 교정했다. (1) `FileSpecDirectoryHooks`의 `root/path`가 `\D\guide*.english`처럼 루트 디렉터리만 있는 패턴에서 게임 폴더를 버리고 드라이브 루트(`H:\D\...`)를 검색해 디스크의 `Guide*.english`(사용자 캠페인)가 한 건도 목록에 오르지 않았다(`root/path.relative_path()`). (2) `BaseFileSystem::MatchFinder`의 결과 경로 접두 비교도 같은 이유로 버려졌다(같은 교정). (3) 원문 PE `4c8c12..36`의 `%` 인라인 목록 삽입이 합친 문자열 전체가 아니라 `replacement` 길이만 복사해 본문 뒤쪽의 목록이 잘렸다. `HtmlWindowMetadataHooks::repairInlineList`(제품에서만 켬, 기본 꺼짐)로 전체와 NUL을 쓰게 했다. | 실제 화면: `Campaign → User Made Campaigns`에 `Legendary Missions Of Ass` 등 목록과 스크롤바 표시 |
| NB09 | **수정 완료**(MP 실행 검증 제외) | 참가자 이름표 `WriteNativeMetaParticipantLabel`이 길드 태그가 길어 CS 이름표 버퍼(`+d8`)를 넘으면 `out_of_range`로 클라이언트 전체가 종료했다(원문은 버퍼 훼손). 태그 끝 글자를 한 글자씩 줄여 서식에 맞추고, 길드 없이도 넘치는 경우만 명시 거부로 남겼다. | 단위 `KnownBug_NB09_LongGuildTagIsShortenedInsteadOfFailing` 통과. 존 실제 오른쪽 클릭 메뉴는 MP라 실행하지 않음(사용자 확인) |
| NB11 | 현행 미재현 | 메인 `Demo`의 `The Storm Rages`·`Demo of "Guard My Back"`을 브리핑 `OK` 뒤 각 3600프레임(약 1분 게임 시간) 실행했다. 번개/캐논 교전이 진행되는 화면이고 종료·로그 없음. `RawAiDamage::Notice`는 brain 부재를 `invalid_argument`로 거부하는 구조라 AI 워크샾 대피해 때의 부재 brain 경로는 여전히 의심 지점이다. | 실제 실행(`extracted/cpp-resume-1278/nb11storm,nb11guard/`). `Dissolved Alliance` 데모와 더 긴 시간·AI 워크샾을 크게 파괴하는 경로는 사용자 확인 |
| NB18 | **수정 완료**(1290, 정적·단위 검증) | 원문 `Carrier.cpp` 포획 수신(`00426520`, 현행 `RawCarrierPickup::Receive`)은 대상이 사제(genus `0x200000`)여도 부모가 이미 든 내용물(`4ac9f0` mask1, 너겟)을 검사하지 않고 두 번째 내용물을 만든다(원문 증상 "사제가 또 다른 너겟으로 변함"과 맞는 경로로 **추정**, 실제 재현은 포획이 얽혀 사용자 확인). 요청(`00428e00`/`Request`)도 같은 조건을 거르지 않았다. `CarrierPickupState::refuseHeldPriest`(기본 꺼짐, 실제 `RawCampaignSession`만 켬)로 요청과 수신 양쪽에서 거부한다. 스위치가 꺼져 있으면 독립 원문 fixture의 호출 기록이 그대로다. | 단위 `KnownBug_NB18_CarrierHoldingContentRefusesPriestPickup` 통과(요청·수신 거부, 빈 손·스위치 꺼짐 정상). 사제 포획 실제 시험은 사용자 확인 |
| NB20 | **수정 완료**(1289) | 원문 10.78의 사제 선택 키 P/R(`004d9884..004d98e1`, 현행 `SelectPriestByKey`)은 건설 준비 조각을 취소하지 않고 사제만 선택했다(이전 시험은 F5=View Your Priest라 선택 키가 아니었고, 시험 하네스가 `priest`/F5 키를 거부해 아예 전달되지도 않았다). 원문 취소 `00446e70`(우클릭과 같은 반환 경로)은 15곳에서 호출되지만 이 키 경로에는 없다. `PriestSelectionShortcutHooks`에 선택 사항인 `pieceHeld`(잠금 깊이 0이고 미리보기 비0)/`cancelPiece`(`Cursor().Cancel()`)를 더하고 `GameWorld::SelectPriest`·`ClientMetaPointerInput`이 연결한다. 장치가 없으면 원문 호출 순서 그대로라 독립 원문 fixture 220건은 변하지 않는다. | 단위 `KnownBug_NB20_PriestKeyCancelsHeldBuildPiece` 통과(`netstorm_known_bug_tests`). 실제 Tutorial4: 바람 발전기를 커서에 건 뒤 `P` → `lockDepth 0→1`·미리보기 59→0, 화면에 사제 선택 표시만 있고 건물 고스트 없음(`extracted/cpp-resume-1278/nb20fix2/`) |
| NB22 | 현행 미재현 | Test Battle에서 `P`/`R`/`F`의 다섯 조합(p,f / r,f / f,p / f,r / p,r,f)을 연속 입력했으나 종료/로그 없음. | 실제 실행(`extracted/cpp-resume-1278/nb22/`). 사용자 확인 잔여: 동시 눌림(chord)·다른 화면 |
| NB24 | **수정 완료** | 긴 캠페인(Tell 파일) 제목이 80BYTE 지역 버퍼를 넘으면 원문처럼 `PhysicalLimitError`로 종료했다. `FitDisplayLimit`로 마지막 UTF-8 문자부터 줄여 표시한다(NB08 교정으로 목록이 열리며 즉시 노출된 같은 증상). | 실제 데이터에 80BYTE 이상 제목(`GuideHades` 86자, `GuideKQ` 102자: 아이콘 마크업 포함)이 있어 NB08 교정 직후 처음에는 `Tell 파일 목록의 원본 지역 NUL 범위 초과`로 종료했고, 교정 뒤 목록이 정상 표시되며 종료·로그 없음 |

## 미조사(원인 위치만 정리)

| ID | 조사 시작점 |
| --- | --- |
| NB01 | 보충 간헐천 AI 연결 능력 확장의 누락 매개변수 — `GeyserAttachments`(AI +230) 소비자와 `randGeysers`/`replenish` 경로(`00486440` 호출자 4곳)를 비교 |
| NB03 | Tutorial4 `production:75` 선택 화면에서 생산 슬롯 표시는 보였다. 워크샾 전용 슬롯 열(다중 슬롯)이 비는 조건은 미확정 |
| NB04/NB28/NB29 | 시작/재생 간헐천 분배식 — `PatchFixs.txt`의 10.75 "시작 간헐천 수는 플레이어 수·맵 크기·간헐천 양을 반영" 항목과 `RawNuggetReplenish`·`00486440` 비교. 수정 기준이 되는 올바른 공식은 원문 자료에 없어 설계 결정 필요 |
| NB06 | AI 신전 인식 — 비영 brain `RawAiBrainMemory`의 신전 탐색/생성 분기 |
| NB10 | 싱글 미션의 잘못된 player side — 미션 Header(`myPlayer`/side) 검증과 `RawMissionStart` 단언 |
| NB12/NB30/NB31/NB34 | MP 전용(입력 중 Battlemaster 시작, fort 적재 전 호스트 종료, 해상도 재시작 로그아웃, 클라이언트 체력 표시) — 디컴파일 대조만 가능하며 MP 실행 제외 |
| NB13 | Cid 부동소수점 반올림(edge suck) — 이동/지형 가장자리 경계 계산 |
| NB14 | 싱글 편집기에서 멀티플레이 fort 로드 — Edit 목록/`ReadFort` 경로의 파일 종류 구분 |
| NB15/NB16/NB17 | 클라이언트 애시드 바리케이드 회전·벌프 줍기/내리기 반영 — 패킷 수신 본체(MP) |
| NB19 | 스톰 크리스털(너겟)을 내려놓은 뒤 200SP — `RawCarrierArrival`의 반환지 도착 처리와 `SquidReward` 환급 경로. "내려놓은 뒤 처리장에 가는" 정확한 조작 순서가 원문 자료에 없어 원인 위치 미확정(사제 포획이 얽히면 실제 시험은 사용자 확인) |
| NB21 | 컬렉터 선택→건설 메뉴 배치 후 선택 상태 — 선택/건설 메뉴 부모 |
| NB23 | 드문 섬 생성 종료 — `RawIslandBuilder`/지형 생성 난수 경로 |
| NB25/NB26 | 썬 캐논 사거리 표시, 썬더 캐논 근접 표적 — `RawCannonFire`/Gun 사거리·표적 선택 |
| NB27/NB32/NB33 | 공중 표적 변경 건설 중 종료, 아이스 캐논의 인접 객체 공격 종료, 에디터 아이스 타워 섬 삭제 — 전투/수명/편집 |
