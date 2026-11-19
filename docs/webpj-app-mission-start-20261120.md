# WB04-E970 앱 미션 Start 준비

로그인 Ready인 현재 World와 동일 토큰의 Guide Script·Runtime에 `RawCampaignMissionStartBindings`를 준비한다. 앱 Client 시계·Renderer 무효화·메뉴 갱신, 같은 Session의 자원과 기술, 세계 간헐천·카메라 및 현재 채팅 창 부모의 명령 갱신을 기존 실제 서비스에 연결한다. 한국어 AI 이름에는 공통 표시 부호화와 확장 문자 길이 제한을 사용한다. 다른 미션 토큰·미완료 로그인·중복 생성은 거부한다.

검증: 실제 Tutorial1 Guide 요청부터 로컬 Ready, 현재 World와 Start 생성, 해당 대본의 시작 SP, 순번 3→4와 started, 실제 Gump 부모의 `0x02000088` 알림을 확인했다. 관련 Node 14건 통과. 검사는 아직 로그인 완료와 Start를 직접 호출했으며 브라우저의 자동 순서가 아니다.

남은 범위: 앱 프레임의 로그인 완료·Guide 3단계 자동 호출, Tutorial 안내 소유자와 실제 미션 Tell·게임 프레임, 메뉴 명령·화면·입력 및 기존 World 교체가 남았다. E966 고정 Node 26 전체 6,547건 통과 후 Node 22 전체 회귀 실행 중이다.
