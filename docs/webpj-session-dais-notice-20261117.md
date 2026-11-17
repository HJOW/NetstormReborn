# Session 다이스 내용물 통지 (WB04-E650)

기준선 `225553df`의 `RawCampaignRitual.inc` 끝에서 `RawDaisContentNotice`를 기존 Session의 Pool·프레임·룬 조회와 내용물 부모 통지 분배기에 연결했다. 현재 사제 TYPE·로컬 owner·다리 TYPE은 호출 때마다 같은 전역에서 읽고, 제단 Unpop/Repop·표면 통지와 명령 `0x020000d7`을 실제 Session 장치로 전달한다. 이전 다이스 수명 설치를 선행 조건으로 삼으며 반납한 함수의 접근을 거부한다.

신규 Session 검사는 현재 전역 변경, 실제 입력 명령 큐, 제거 이유의 자연 반환, 설치 순서와 반납을 확인한다. 원본 `daiscontentnotice` fixture의 10.78 행을 대조하는 검사 등을 포함해 집중 15건을 통과했다. 사제 포획은 재현하지 않았으며 내용물 통지 본체의 독립 fixture 비교와 실제 자연 플레이를 구분한다. 회수·운반 명령, 전체 Load 및 `loaded=true`는 후속이다.

Node v22.23.3·v26.13.0 전체 각각 6,071건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e650-node22.log`·`extracted/webpj-e650-node26.log`다.
