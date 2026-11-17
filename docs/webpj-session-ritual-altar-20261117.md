# Session 제단 정기와 해방 처리기 (WB04-E648)

기준선 `225553df`의 `RawCampaignRitual.inc` `ConnectDaisLifecycle` 앞단에서 제단 `RawFrameAdvance`와 `RawAltarSacrifice`를 같은 Session의 Pool·해시·프레임·신전·룬·운반·피해 전처리·Host에 연결했다. 원본에서 이 파일은 기준선 뒤 변경되지 않았다. 게임 난수, 현재 가상 개수와 권한, World의 Unpop/Repop/Pop·소유자, 위치/전역 음향, 공황 회피와 Player 갱신을 실제 Session 장치에 연결한다. 원문과 같이 Regular를 한 번 확보하고 같은 객체를 공유 타입 61로 Host에 부착한다. 설치 순서·중복과 반납을 명시적으로 확인한다.

`raw-campaign-session-ritual.test.mjs`의 신규 검사는 선행 다이스 없이 설치 거부, 동일 Pool·현재 권한, 같은 Regular 객체의 예약/중첩 거부/Host 부착, 반복 설치와 반납을 확인한다. 제단 원본 관측 `altarsacrifice` 1,435행 및 `daisprogress` 1,056행과 부품 계약을 포함한 집중 검사 27건이 통과했다. 다이스 최초 Pop/삭제 후처리와 내용물 통지, 회수·운반 명령, 전체 `Load`는 이 단위 밖이며 `loaded=false`다. 자연 희생과 실제 미션은 실행하지 않았다.

Node v22.23.3·v26.13.0 전체 각각 6,069건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e648-node22.log`·`extracted/webpj-e648-node26.log`다.
