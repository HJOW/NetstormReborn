# Session 다이스 최초 Pop과 삭제 후처리 (WB04-E649)

기준선 `225553df`의 `RawCampaignRitual.inc` `ConnectDaisLifecycle` 중 다이스 수명 생성·후처리 등록을 같은 Session의 Pool·해시·신전·제단·운반·피해 전처리와 연결했다. 로그인 종료 시각은 실제 장기 상자를 빌리며, Player 기록·현재 권한/가상 개수·프레임·난수 예약·월드 좌표/생성·설정/Tell 및 피해 삭제 후처리를 실제 장치에서 읽는다. 같은 Core PostPop에 등록하고 반납 시 등록을 제거한다. 제단의 로컬 정기 21은 앞서 확보한 같은 객체를 타입 46으로 Host에 부착한다.

신규 Session 검사는 실제 월드에서 제단 Pop → Host 정기 예약, 다이스 Pop → 현재 Player SID 두 칸 등록, 현재 로그인 시각 대여, 설치 순서를 확인한다. 원본 10.78 `daislifecycle` 964행을 독립/실제 Player 기록 양쪽과 대조하는 검사 포함 집중 30건이 통과했다. 내용물 부모 통지와 명령·전체 Load는 후속이며 `loaded=false`다. 자연 희생·실제 미션은 실행하지 않았다.

Node v22.23.3·v26.13.0 전체 각각 6,070건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e649-node22.log`·`extracted/webpj-e649-node26.log`다.
