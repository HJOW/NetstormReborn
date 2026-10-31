# 곡 만료 시 현재 battle 조회 — 단위673 준비본

2026-11-01 / HJOW-Athlon. 사용자의 현재 작업 완료 후 일시 중지 요청에 따라 이미 진행 중이던 준비만 마쳤다. **제품 미적용·C++ 미빌드·새 검사8개 미실행**이다. 단위672의 제품/검사 통과 수에 포함하지 않는다.

원문 `00469f60`은 곡 만료 조건 뒤 현재 `00594fbc`를 읽는다. 기존 제품은 cleanup의 StartScene이 복사한 battle1을 보존해 실제 owner가0인 메뉴에서 다음 만료 시 잘못 Next를 고를 수 있다. 원문 cleanup의 음악 호출→battle0 순서는 유지해야 한다. State3의 직접 ser22 요청은 Start(false) 재호출과 다르며 색인3을 다시 쓰지 않는다. 따라서 메뉴 번개 검사는 시작 색인을 고정하지 않고 실제4곡 순환과 부모 부재를 확인한다.

준비 API는 선택적 `SceneMusicState.currentBattle`, `SceneMusic::CurrentBattle`, `ClientAudio::BindCurrentBattle/CurrentBattle`와 진단의 같은 현재 조회다. Start의 첫 조건과 Frame의 만료 조건 뒤에만 읽고 Next에는 관문을 추가하지 않는다. 독립/10.72 fallback과 기존 StartScene(bool) 쓰기는 유지한다.

**실제 Client 연결과 CMake 등록은 준비하지 않았다.** 실제 Run은 음향 초기화 뒤 SurfaceStorage를 생성한다. 같은 authoritative SurfaceStorage가 설치된 뒤, 최초 음악 Start보다 앞에서 10.78의 현재 ownerMode.battle을 연결해야 한다. 미설치 무장치 장면에 fake0 reader를 붙이거나 필수 소유자 guard를 제거하지 않는다. 기존 Client·State·Guide 수명은 유지한다.

보관물은 다음과 같다.

- `raw/base/`: 단위672 작업본의 실제 입력4파일이다. 빌드 대상이 아니다.
- `raw/proposed.patch`: 제품4파일 변경과 신규 독립 검사1파일/8개 초안이다.
- `raw/manifest.json`: 입력/제안 SHA와 미구현 연결·미실행 한계다.
- `raw/scene-menu-music-audit.md`: 원문/현행 정적 근거다.
- `raw/startup-validation-audit.md`: Auto Updater 검증 안내는 기본 무인자 시작의 정상 결과이며 싱글/LAN을 막지 않는다는 별도 분류다.
- `archive-files.json`: 원시 보관물의 길이와 SHA다.

재개 시 AGENTS.md와 LEFT_JOBS.md를 다시 읽고 호스트/마지막 정밀 디컴파일을 확인한다. 현재 소스와 manifest 입력 SHA를 대조한 뒤 별도 디렉터리의 base에 patch를 적용해 제안 SHA를 확인한다. 바뀐 제품에 무조건 덮어쓰지 않는다. 이번 보관 검증은 patch 재현만 하며 빌드·원문 실행·기능 검증을 대신하지 않는다. 실제 Client 연결과 관련 검사·문서·커밋은 재개 후 작업이다.
