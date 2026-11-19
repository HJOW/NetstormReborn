# SoundPlayer Web Audio 버퍼 장치 (WB04-E963)

장치 독립 `SoundPlayer`에 연결할 Web Audio 버퍼 장치를 추가했다. 미리 해독한 같은 `AudioBuffer`를 논리 버퍼별로 빌리고, 재생 때마다 새 `AudioBufferSourceNode`를 만들어 단발·반복 상태와 자연 종료를 추적한다. 버퍼 복제, 정지와 위치 0 초기화, 범위 밖 음량·좌우 실패, 소리 파일 없음 표식을 원본 장치 경계에 맞춰 제공한다. 화면 좌우 값은 반대쪽 채널을 DirectSound 감쇠량으로 줄이는 두 gain 노드로 적용한다.

실제 `SoundPlayer`·`SoundList`·음향 파일 선택/해독 경계를 연결한 Node 검사에서 복제 항목, 감쇠 파일 500, 자연 종료 후 재생 수 재계산, 반복 정지와 장치 반납을 확인했다. Chromium `OfflineAudioContext`로 모노 표본을 실제 렌더링해 전체 -1500 및 왼쪽 추가 -2000 감쇠의 좌우 출력 샘플을 확인했다. 연관 Node v26 검사 10건과 Chromium 1건이 통과했다. 실제 스피커 청취는 하지 않았다.

이 장치는 `AudioSystem.soundGain`을 중복 적용하지 않도록 문맥의 destination에 직접 연결한다. 따라서 앱은 실제 사용 전에 `SoundPlayer` 전역의 활성화와 전체 음량을 설정 옵션과 동기화하고, 필요한 효과음 파일을 비동기 준비한 뒤 Session에 연결해야 한다. 음악 선택과 미션 시작·종료에 따른 정리도 별도 작업이다. E958 기준 Node v26·v22 전체 회귀는 각각 6,529건 모두 통과했고, E962 기준 Node v26 정기 회귀가 별도 작업 트리에서 실행 중이다.
