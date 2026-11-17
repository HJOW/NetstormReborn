# Session 신전 의식과 룬 후처리 (WB04-E646)

기준선 `225553df`의 `RawCampaignRitual.inc` 69..70과 80..95를
기존 Session의 사제 의식·Pool·World·Factory·Host·TYPE·지형과 결선했다.
관련 cpppj 소스는 기준선 뒤 변경이 없다. 신전의 현재 사제 TYPE과 권한은
공유 getter로 읽고 생성·HP·소유자·Pop·음향·307 공유 예약은 기존
같은 소유자를 사용한다. 룬은 실제 postPop 분배에 등록하고 동일 예약으로
304 일반 Regular를 붙이며 현재 건설 시각·Player 플래그를 사용한다.
Host를 비운 뒤 룬 분배를 반납한다.

신규 검사 2건은 같은 Pool과 현재 TYPE/권한, 중복 설치, 실제 룬 Pop에서
같은 Host의 304 예약과 payload·일반 유형을 확인한다. 신규 검사는 첫
실행에 통과했고 원본 직접 소비를 포함한 집중 검사 35건이 통과했다.
원본 신전 3,360행 fixture SHA-256은
`6ad3c0abef4a2af956e949ae54e7005d89811c0287bd9d5328e94efa3a98be65`,
룬 960행 fixture는
`784669b8c4c20b725b3e23492bc76e4b1a4ab21042dfb9850378907bafd35e99`다.

Node v22.23.3·v26.13.0 전체는 각각 6,064건 통과, 실패·취소·건너뜀 0이다.
v24는 사용자 지시대로 생략하고 호환을 가정한다. 대응표 1,160행 오류 0이다.
전체 로그는 Git 제외 `extracted/webpj-e646-node22.log`와
`extracted/webpj-e646-node26.log`에 보관했다.

다이스·제단·희생 요청과 실제 미션 `Load()`·자연 의식 진행은 아직
미연결이다. `loaded=false`를 유지하며 게임이나 브라우저는 실행하지 않았다.
