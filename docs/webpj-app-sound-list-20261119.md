# 앱의 미션 효과음 이름표 (WB04-E949)

기준선 `225553df`의 `ClientAudio`는 Session보다 오래 사는 `SoundList`를 소유하고, `SoundDevice::Initialize`는 장치 개방 전에 이름표를 초기화한다. 웹 앱도 Client 초기화 때 같은 이름표와 대체 항목을 준비해 Session들에 대여하고, World 반납 이후에도 앱 종료까지 보존한다. 브라우저는 사용자 입력 전 오디오 재생이 불가능하지만 이름표 초기화는 장치를 열거나 소리를 재생하지 않는다. 새 앱 수명에서는 새 표를 만든다.

기존 실제 Session 교체 검사에서 두 Session이 같은 Client 표를 사용하고 첫 Session 생성에서 추가한 항목이 반납 뒤에도 남는지 확인했다. 두 실제 Guide World 설치 및 로컬 로그인 검사에서는 검사 전용 `SoundList`를 넘기지 않고 앱 소유 표를 사용했다. 현재 표의 토큰을 Web Audio 재생·위치 음향과 연결하는 작업은 아직 남아 있으며 이 변경은 실제 미션 소리 재생을 뜻하지 않는다.

검증: 실제 Session 교체·World 설치·로그인 3건과 로그인·팔레트 연관 검사 10건이 Node v26.13.0에서 통과했다. 정기 Node v26 전체 회귀는 `extracted/webpj-e949-node26.log`에 실행 중이며 종료 결과를 후속 단위에서 확인한다.
