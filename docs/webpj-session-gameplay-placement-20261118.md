# Session 게임 배치 판정 설치 (WB04-E693)

기준선 `225553df`의 `RawCampaignSession.cpp` 3576..3594를 현재 Session의 게임용 `RawCanonPlacementPipeline`에 연결했다. 지형 생성용 판정기와 별개로 같은 실제 Pool·해시·TYPE·섬 기록을 빌리고, 매 요청에서 Player·동맹·Graph·건설 전역·섬 제한과 존재를 반영한다. Native 장기 소유자 분기는 4D-8에서 다룬다. 관련 C++ 소스는 기준선 이후 변경이 없다.

기존 `MayPlace` 앞단이 기준점 상태 객체를 통째로 교체해 이미 생성한 판정기가 낡은 객체를 참조하는 결함을 고쳤다. 객체 정체성을 유지하면서 현재 필드를 복사하고 비공개 FullDeck getter도 현재 원본에서 읽는다. 최초 전체 회귀는 기존 검사가 잘못된 객체 동일성을 기대하여 한 건 실패했다. 원본의 값 복사 계약을 검증하도록 교정한 뒤 실제 유닛 생성·포획·희생을 실행하지 않는 연결 검사와 기존 10.78 배치 관측 fixture 144행 등 집중 15건이 통과했다. 커서와 실제 `Load`는 후속이며 `loaded=false`다.

Node v26.13.0 재실행 전체 6,121건 통과(실패·취소·건너뜀 0). 최초 실패도 Node 22 주기에 포함해 E691의 Node 22 통과 이후 3/8회다. 로그는 Git 제외 `extracted/webpj-e693-node26.log`와 `extracted/webpj-e693-node26-rerun.log`다.
