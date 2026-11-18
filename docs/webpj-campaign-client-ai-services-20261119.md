# WB04-E838 캠페인 Client AI 서비스

기준선 `225553df`의 `BindCampaignAiClientServices`를 같은 Client의 현재 언어·표시 인코딩·설정·채팅 장부에 연결했다. 일반 미션에서는 적 AI에 현재 difficulty를 사용하고, 스커미시 실행 상태에서는 플레이어 수를 1~7 AI 범위로 제한해 계산하며 2~8번 AI의 개별 난이도를 읽는다. AI 이름은 현재 번역·설정 치환을 사용하고 실제 채팅 Add/Clear에 전달한다. Client와 장부는 Session보다 오래 살아야 한다.

필요한 `ConfigInterface::SubstituteLocal`을 GameData에 옮겼다. 임시 local 객체에 최대 세 인자를 넣어 본문을 치환하고, 예외가 나도 등록을 해제한다. 실제 GameData와 RawChatChannels로 언어 변경, 이름, 임시 치환·예외 정리, 채팅, 일반·스커미시 난이도와 인원 제한을 검사했다. 표시 부호화는 실제 Client가 제공할 장치 경계이며 검사에서는 원본 1252 인코더를 사용했다.

실제 GuardMyBack 내부 Login/Start에서 같은 Client 설정을 쉬움으로 두고 적 AI가 그 값을 선택하고 채팅 장부를 갱신한 뒤 State 정리가 끝나는지 확인했다. 진단 검사 중 새 난이도 경로에서 데모 판정 장치가 필요해 실제 일반 미션 값 0을 명시했다. 실제 Client 소유·스커미시 UI와 나머지 서비스는 아직 남는다. 게임 창은 실행하지 않았다.

검증: Client 효과·설정 집중 8건과 실제 AI 미션 통합 1건, Node v26.13.0 전체 6,336건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. E835의 Node v22 전체 검사 뒤 v26 회귀 3/8회다.
