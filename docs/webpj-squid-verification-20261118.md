# WB04-E730: 객체 검증 시작·free 응답

cpppj `RawSquidVerification.cpp/.h` 기준 `147d4a39`의 4bh 검증 시작, 4ah free 응답, 이전 SID와 현재 SID 사이의 순회를 이식하고 실제 Session 등록에 연결했다. `GameAuxState`의 0054d4d8 현재 검증 SID와 새 0054d4dc 이전 순회 위치를 공통 객체 수신과 공유한다. 순회는 DWORD 증가·signed 비교·15000 wrap을 따르며 현재 슬롯과 로그 효과 뒤 다시 읽은 TYPE을 비교한다.

일반 독립 Pool에서 진단 복귀 뒤 입력 재조회, 현재 슬롯 재조회, wrap 및 free 응답의 SID 정리를 대조했다. 실제 Tutorial 1 Session은 사용하지 않은 빈 슬롯만 이용해 4bh→4ah 등록과 공유 DWORD를 확인한다. 공중 3종 전용 생성·생산·우클릭·행동, 사제 포획·희생·자연 승리는 실행하지 않았고 실제 게임이나 브라우저 창도 열지 않았다.

이 수신 본체와 실제 Session 연결은 완료했다. 객체 변경 2dh·2eh·65h, 압축 7ah와 그 밖의 Carrier 등록, 서버 응답 왕복과 앱 미션 수명은 남았다.

실제 Session·독립 Pool·GameAux 전역 검사를 함께 실행해 33건이 통과했다.

대응표 오류 0. Node v26 전체 회귀 종료 뒤 Node v22를 단독 실행했고 두 버전 모두 6,196건이 통과했다. 다음 Node v22까지 v26 주기는 0/8회다.
