# 실제 TheWarBegins(캠페인 1-1) Session Load (WB04-E697)

기준선 `225553df`의 `RawCampaignSession::Load` 4343..4351을 실제 건설·이동 설치, `RawCampaignWorldRuntime.load`, 주문·운반·기도·건설 사제·배치·커서 설치 순서로 연결했다. 모든 단계가 끝난 뒤에만 `loaded=true`이며 실패 시 앞선 효과와 미완료 상태를 보존한다. 같은 소스와 `RawAssetPostPop.cpp`는 기준선 이후 변경이 없다.

보호된 실제 TheWarBegins(캠페인 1-1) Fort·TYPE·SHP·팔레트를 읽어 별도 게임 실행 없이 Node 내부에서 같은 Session을 구성했다. UI·음향 경계는 관찰하고 명령은 실제 `InputQueue`에 담아 적재했으며, 실제 저장 객체 1,875개와 플레이어 1 사제 SID 15000, 예약 명령 9개를 관찰했다. 원본 cpppj `CampaignSession_ActualWarBeginsGameWorld`의 사제 존재·SID 계약을 확인했고 재진입·중간 실패도 검증했다. 앱의 미션 진입·프레임·화면 연결이나 자연 플레이를 뜻하지 않는다.

최초 적재에서는 주입해야 하는 생산 갱신·음향·명령 큐 장치가 빠져 중단됐다. 실제 명령 큐와 관찰 장치를 제공한 뒤에는 Gun 후처리가 Session의 숫자형 `checkingForms`를 함수로 호출해 실패했다. 실제 값을 읽는 시점에서 숫자와 기존 함수 입력을 모두 지원하도록 고쳤다. 새 Load 검사 2건과 기존 월드 로더·커서·자산 후처리 검사를 포함해 집중 42건이 통과했다. 포획·희생·자연 승리나 공중 3종 전용 행동은 실행하지 않았다. Native 장기 분기, 앱 수명, session-dump와 실제 미션 화면 비교는 후속이다.

Node v26.13.0 전체 6,129건 통과(실패·취소·건너뜀 0). E691의 Node 22 통과 이후 완료된 전체 회귀는 7/8회다. 로그는 Git 제외 `extracted/webpj-e697-node26.log`다.
