# 단위871 — 현재 너겟 번호의 공통 소비자와 장기 참조

내용물·운반·채집·건설·다이스·경로의 너겟 타입 번호를 869의 보상과 같은 Client 장기 `005412c0` owner에 연결했다. 실제 의식/포획/희생·게임/통신은 실행하지 않는다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**, 현재 호스트도 일치하며 산출물은 변경하지 않았다.

## 연결

`ContentLifetimeState`, `CarrierArrivalState`, `CarrierTransferState`, `ConstructionPlaceState`, `DaisRegularState`, `PathCleanupState`, `GeyserMinedState`가 같은 현재 너겟 번호를 선택적으로 빌린다. `NuggetType()`은 const 참조를 반환하며 별도 연결이 없으면 기존 `nuggetType` 필드를 사용한다. 따라서 독립/CD 입력과 이미 참조로 보관하는 장치의 수명 계약을 유지한다.

내용물은 커서 취소 뒤 kind를 판정하고, 수입 알림·지갑 선가산 뒤 단가를 각각 다시 읽는다. 채집은 위치 음향 뒤의 현재 번호로 TYPE float 단가를 골라 수량 WORD를 줄인다. 건설은 기존 로컬 Player/비용 관문 뒤 너겟 비용 예외를, 환불은 해당 함수의 현재 번호 예외를 읽는다. 이 순서는 기존 몸체에 유지하고 번호의 원천만 바꿨다.

실제 Session에서 내용물·전달·채집과 운반/경로 정리·건설을 보상의 같은 owner에 연결했다. 늦게 생성하는 다이스 상태도 `RawCampaignRitual.inc`에서 동일 주소를 빌린다. AI의 내용물 수량/첫 너겟 조회, 전체 덱 복원/명령, 경로 재지정과 회수, UI getter 및 의식 요청의 기존 참조 인자도 현재 getter로 연결했다. Prepare 단계에서 다른 state의 번호를 Geyser에 복사하던 대입은 제거했다.

원문 `DAT_005412c0` 소비자22함수와 CD 대응 `DAT_0051cbac` 소비자15함수를 읽기 전용으로 추출해 보존했다. 이는 해당 함수 전체37개를 이번 단위에서 새로 구현/실행했다는 뜻이 아니다. 최초 정수 비용 캐시와 내용물/채집의 현재 TYPE float 단가도 서로 구별한다.

## 검증

첫 제품 빌드는 종료1·126.903초이며 `C2039` 한 곳을 보존했다. `constructionState.confirm`은 확정 상태라 새 멤버가 없었다. 실제 `ConstructionPlaceState`인 `constructionState.place`로 바인딩 한 곳을 교정했다. 검사 소스/기대값은 변경하지 않았다. 교정 뒤 제품/두 집중 대상 Release는 **종료0·407.146초·경고/오류0**이다. 교정 스냅샷21소스와 최종 SHA-256이 같으며 추가 교정은0이다.

집중 검사는 **고유17개/12묶음(새4), 첫 검사 실행 실패0**이다. 새 내용물·채집·건설 부품3개는 커서 취소/안내/지갑·음향·비용 관문 뒤의 현재 kind/단가/제외를 확인했다. 실제 Session1개는 Load 뒤 getter만 조회하고 원래 번호를 복원했다. 요청 명령/패킷·다이스 획득/희생 효과는 실행하지 않았다. 기존 공통 채집2·내용물/SP/혼합 수명2·건설 비용2·경로 재지정2·보상 Session1·Rune 참조/Session4도 통과했다. 혼합 대상의 기존 패킷/전송/의식 검사는 선택하지 않았다.

새 Native/fixture·기존 기대값 변경0, 보호2761개/fixture941개 전부 불변이다. 제품871/11.01, EXE 10592768바이트, SHA-256 `d65bd8275e252becb5cea3fcbcb7669ccdbbd85a1aed1d1cf34f471bd21491e7`다. checking10(부모4/부분3/MP3)·완료부모38/부분3 유지이며868 공중 전용 실행 검사는 여전히 생략이다.

세부 해시는 [JSON](../../cpppj/nugget-type-consumers-20261105.json), 첫 실패/교정 소스·빌드 로그·보호/검사 목록은 Git 제외 `extracted/cpp-resume-871/`에 보존한다. 다음872는 생산 내용물 정보의 현재 번호와 너겟의 실제 VT 가상 분배다. 870의 InventoryAboutMenuTypes 임시명은 실제 InventoryAboutState로 정정한다. 현재 owner는 이미 공개된 `RawSurfaceStorage::GameAux()`에서 빌릴 수 있다.

`RawNuggetRegular`/Session의 TYPE+VT 가상 판별, `InventoryAboutState`의 번호 사본과 다른 현재 TYPE/모드·전체 Pop/Factory/군도·메뉴/Edit/State/파일·Windows·후속 단계는 별도 잔여다. 구조체 바이트 오프셋/고정 색표의154는 TYPE 번호로 바꾸지 않는다. 실제 게임/GUI·특정 적 공격/승리·공중3종 전용·물리/PCM/CD 재시도와 전체71/CTest/34미션/ZIP 반복도 제외한다.
