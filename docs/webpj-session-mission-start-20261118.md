# WB04-E758 실제 로그인 뒤 미션 시작

기준선 `225553df`의 `RawCampaignSession.cpp` 현재 Player·사제·기술·미션/AI 설정 공개 경로를 같은 Session의 실제 소유자에 연결했다. 기존 `RawCampaignMissionStartBindings`가 사제 목록을 함수로 잘못 가정하던 계약을 실제 `RawPriestWorldBindings`의 속성에 맞췄다. 대체 월드나 임시 기술 장부는 만들지 않는다.

`CampaignSession_LocalLoginMissionStartAiAlliances`에 대응해 보호된 GuardMyBack 실제 스크립트와 Fort를 읽고 내부 로그인 후 일반 및 데모를 독립 Session에서 시작했다. 두 경우 시작 자금 1,200, 기술 TYPE 133, 적 Player 2·3·4의 AI, 일반에서 로컬 AI 없음과 데모에서 로컬 AI 존재, 시작 시각 123.5·데모 기한 483.5, 간헐천 옵션, UI 갱신 순서와 전송 전 동맹 패킷 부재를 확인했다. Pump 뒤 9바이트 3dh 동맹 패킷과 같은 Player 동맹 장부를 검증했다. 게임 프레임이나 AI 행동은 실행하지 않았다.

`cpppj/src/client/RawCampaignSession.cpp`, `RawCampaignMissionStartBindings.cpp`, `RawMissionStart.cpp`는 기준선 이후 변경되지 않았다. `CampaignSessionTests.cpp`의 기준선 이후 변경은 표시·공유 Kernel 수명이며 이번 대응 검사 구간은 변경되지 않았다. 기존 MissionStart 덕타입 검사 11건과 실제 로그인 통합 검사 1건이 통과했다. 새 fixture와 게임 창 실행은 없다.

사용자 커밋 `faecb9cb`가 작업 중 15차 설정 초기화 요구를 추가했다. 웹의 확인 후 초기화·사용 해제·재로딩을 인수인계 0.6·3.7절의 미완료 항목에 반영했다.

후속 점검에서 E756의 다리 주기 16진수 변환은 `0x1.0000010000000001p0`를 1로 잘못 반올림하는 반면 현재 Windows UCRT는 1.0000001192092896을 반환함을 확인했다. 다음 단위에서 단정도 반올림을 교정해야 한다.

Start 호출은 아직 웹 Client의 미션 수명이나 브라우저 화면에 연결되지 않았다. 다른 로그인 통합·선택 UI·미션 프레임·앱 연결, 실제 플레이와 자연 완주는 남아 있다. 포획·희생·특정 적 공격·승리 및 세 공중 유닛 전용 행동은 검사하지 않았다.

Node v26.13.0 전체 회귀 6,257건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E753의 v22 검사 뒤 다섯 번째 v26 전체 회귀다.
