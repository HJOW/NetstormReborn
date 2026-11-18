# WB04-E813 State Template·TYPE·시드 전역

기준선 `225553df`의 State 전역 중 현재 Player 물리, Template 9개 헤더의 manager DWORD, 실제 TYPE 비용 float 참조, Fort 핵심 시드의 하위 WORD 저장을 연결했다. Player는 호출마다 현재 base를 읽고 TYPE 비용은 같은 레코드에 float32로 저장한다. 시드의 상위 WORD는 보존하고 잘못된 주소·슬롯·TYPE은 거부한다. 기준선 이후 관련 cpppj 코드 변경은 없다.

실제 `ClientPlayerRecords`·`CampaignTemplateStorage`·`ClientRandomGlobals`·`RawSurfaceStorage`로 빈 Player와 초기화된 현재 물리, 헤더 슬롯·비용 공유·시드 폭을 확인했다. 최초 집중 검사에서 초기화 전 빈 Player 뷰가 호출마다 새 빈 객체인 계약을 잘못 가정해 실패했고, 초기화 전 길이와 초기화 후 실제 공유 뷰를 나누어 확인했다. Audio·메뉴 팝업 및 double 접근과 실제 State 부모 결선은 남았다.

검증: 집중 검사 통과, 대응표 1,160행 오류 0, 관련 난수 fixture 추적 항목을 선택 갱신했다. Node v26.13.0 전체 6,301건 통과(실패·취소·건너뜀 0). E811 Node v22 전체 검사 뒤 v26 회귀 2/8회다.
