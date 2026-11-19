# WB04-E965 앱 미션 효과음 장치

앱 Client가 기존 SoundList와 동일한 수명에 SoundPlayer·WebSoundDevice를 소유하고, Session 효과음과 신전 PostPop 효과음을 그 재생기에 전달한다. 실제 Web Audio가 있으면 현재 언어의 효과음을 먼저 해독하고, 오디오가 비활성 또는 미지원이면 재생하지 않는 장치를 사용한다. 동시에 시작한 준비 요청은 하나의 작업을 공유하며 실패와 반납을 호출자에게 전달한다.

효과음 켜기, 음량 단계와 좌우 교환은 현재 Settings 변경에 즉시 반영한다. 원본처럼 음량은 SoundPlayer의 전체 감쇠로 적용하고 WebSoundDevice는 destination에 직접 출력하므로 기존 메뉴 soundGain과 중복되지 않는다. 현재 화면은 카메라 snapshot과 World 지도 영역을 사용하며, World 이전에는 논리 화면 전체로 대체한다. 설정 `maxSimulSounds`는 기본 8이다.

검증: 앱 재생·옵션 변경·비활성 장치·동시 준비·준비 중 반납, 실제 Tutorial1 State7부터 로그인 완료와 현재 음향 화면, WebSoundDevice와 전체 효과음 준비를 포함한 Node 15건 통과. 오디오 API는 관찰용 장치이고 실제 효과음 전체 해독과 Chromium 출력은 E964에서 별도로 확인했다. E962 고정 Node 26 전체 회귀 6,539건 통과, 실패·건너뜀 0 (`extracted/webpj-e962-node26-isolated.log`).

남은 작업: `_prepareCampaignAudio()`를 브라우저 미션 진입 전환에 자동 연결하고 실패·취소·재진입 UI를 처리해야 한다. 실제 프레임에서 재생 수 재계산, 미션 음악·바람 및 다른 원본 음향 소유자, 완성된 HUD 기준 지도 영역과 실제 청취는 아직 미연결 또는 미검증이다. 이 단위만으로 브라우저 미션 플레이가 완성된 것은 아니다.
