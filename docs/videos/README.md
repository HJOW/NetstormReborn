# 원본 플레이 영상 자료

영상별 관찰 노트(`docs/videos/<이름>.md`)의 공통 전제와 프레임 추출 방법을 정리한다.
영상 목록과 YouTube 주소는 LEFT_JOBS.md 5단계 표에 있다.

2026-10-01 새 YouTube 도구로 [캠페인 1-1~4의 공중 공격체 표본](youtube-whirligigs.md)을 확인했다.
Whirlibase·Whirligig의 출격·공격·귀환을 [클론에 연결](../gameplay/flyers.md)했고,
영상에서 확인한 것과 도움말 규칙·구현 추정을 나누어 기록했다.

2026-10-01 [캠페인 1-5의 사제 포획·알타 의식](youtube-sacrifice.md) 타임라인을 측정하고,
관찰 시점 10개의 프레임 SHA-256 근거와 시각 정정 메모를 `youtube_note`로 저장했다. 코어의 포획·의식·승패 규칙은
[sacrifice.md](../gameplay/sacrifice.md)에 구현 근거와 추정값을 분리해 적었다.

2026-10-01 [캠페인 3-4 구조 미션](youtube-rescue.md)에서 미션 스크립트의 `allowAnyCapture`와 플레이어 섬에 프리스트를 내려놓은 뒤 성공하는 흐름을 영상 프레임으로 대조했다. 이 포획 예외는 해당 미션에만 적용된다.

2026-10-03 [30FPS 사용자 녹화(`record-play`)](record-play-edit-test01-20261003.md)로 **화면 이동 방법**(Alt·가운데 버튼의 거리 비례 속도, 미니맵 끌기), 커스텀 맵 **TEST01 편집기 진입**(Create New Map에 기존 이름 입력), **시험 전투 진입·나오기**를 판독했다. 사용자 녹화가 처음으로 30FPS(평균 29.96)를 달성했다.

같은 녹화의 [화면 요소 복원](test01-visuals-20261003.md)을 [클론 실행으로 후속 검증](test01-verification-20261003.md)했다. 타입별 소유자 색과 `myTech=all`을 수정하고 4:3·16:9·16:10 화면, 한국어·영어, 선택·미니맵·배치 전 캐논 회전을 확인했다. 썬 캐논만 목표를 따라 회전하고 아이스·썬더 캐논은 설치 방위를 유지한다는 사용자 설명도 반영했다.

추가로 [녹음·입력에 근거한 버튼음과 사거리](record-play-details-20261003.md)를 구현했다. 메인 메뉴·브리핑의 `button.wav`, 선택된 공격 건물의 원형·직선 범위, Crossbow의 배치 회전·60도 V 표시와 사격 제한을 연결했다.

[웹 팬게임 참고 전투 보강](fangame-combat-20261003.md)은 로컬 소스의 태양 캐논 전이표를 원본 바이너리와 교차 확인하고 녹화의 탄 외관을 다시 판독했다. 조준 완료 후 발사·이동 회피·원본 탄/폭발 그림과 효과음을 보강했으며 팬게임 참고값과 원본 확정 근거를 구분했다.

## 추가된 YouTube 파일 (2026-10-01)

사용자 추가 파일 `playingVideos/[Youtube] 3-1 to 3-5.mp4`는
[이 영상](https://www.youtube.com/watch?v=0p7VvzSxTAY)의 다운로드본이다.
H.264 1280×720·30fps, 1:41:33.5, **오디오 트랙 없음**을 ffprobe로 확인했다.
전체 2분 간격 표본과 8분·80분 전투 구간을 관찰해 Vander Tower 번개를 클론에 반영했다.
[관찰 노트·재현 명령](youtube-act3-combat.md), [전투 구현·추정](../gameplay/combat.md).
이 파일에는 아래의 기존 방송 영상용 좌우 240px 잘라내기를 적용하지 않는다.

## 기존 방송 영상 (`playingVideos/`, git 제외)

원본 게임을 최대 해상도 1024×768 **풀스크린**으로 두고 16:9 모니터에서 플레이하며 방송 중에 녹화했다 (AGENTS.md).
아래 4개 파일은 2026-09-28 환경에서 확인한 목록이다. 이번 분석 환경에는 이 4개 파일이 없고,
`record-play` 세션 폴더와 위의 새 YouTube 다운로드 파일이 있다.

| 파일 | 미션 | 길이 | 크기 |
|---|---|---|---|
| `Netstorm Islands at war - Early Missions.mp4` | 튜토리얼 1~6 | 37분 56초 | 약 4.0GB |
| `Netstorm Islands at war - The War Begins! ~ Fragile Fortune.mp4` | 캠페인 1-1~4 | 55분 17초 | 약 6.1GB |
| `Netstorm Islands at war - Thundering Power!.mp4` | 캠페인 1-5 | 15분 50초 | 약 1.7GB |
| `Netstorm Islands at war - Dissolved Alliance.mp4` | 캠페인 1-6 | 30분 37초 | 약 3.5GB |

* 형식 (4개 모두 `ffprobe` 확인, 2026-09-28): 영상 **AV1 1920×1080 60fps**, 소리 AAC. 이 PC 의 FFmpeg 는 AV1 디코더 `libdav1d` 를 포함한다.
* **게임 화면 영역 = x 240~1679, y 0~1079 (1440×1080)**. 좌우 검은 여백 각 240px. 4개 프레임(Early Missions 120·600·1200초, Dissolved Alliance 900초)의 밝기 경계로 측정했고 계산값과 같다.
* 배율 1080/768 = 1.40625 (정수배 아님). 영상 좌표 → 게임 좌표: `x = (영상x − 240) / 1.40625`, `y = 영상y / 1.40625`.
* 색: 창 모드 스크린샷보다 밝고 채도가 높다. AGENTS.md(2026-09-28 추가)에 따르면 **평소보다 밝게 촬영되었고 HDR 설정 문제로 추정**된다. 원본 팔레트 색과 다르므로 **색·팔레트 비교에는 스크린샷을 쓰고**, 영상에서는 밝기 차이(변화 감지)만 이용한다.
* 소리: 방송 음성이 섞여 있다. 효과음·음악 시점은 참고용.

## CD 동봉 인트로 (`originalCD/MOVIE/`)

`originalCD/`는 현재 분석에 쓰는 `originals/`의 10.78보다 이전인 **10.72 CD판**의 내용이다(판본·호환성은 사용자 제공 정보). `MOVIE/`에 영문 `englishintro2x.smk`와 독문 `germanintro2x.smk` 인트로 파일이 있다. 이 파일들은 사용자가 녹화한 `playingVideos/`의 게임 플레이 영상과 구분한다. 파일의 실제 영상·음성 내용과 재생 형식은 아직 분석하지 않았다. CD판 실행 파일은 Windows 98/ME 호환, XP에서 실행 불가였으며 Windows 10/11에서도 실행되지 않을 것으로 예상된다. 영상 자료 검토에 CD판 게임 실행은 필요하지 않다.

## 쓰임새

* 좌표·스프라이트 정밀 대조는 스크린샷을 우선한다 (영상은 1.40625배 확대로 픽셀 경계가 흐리다).
* 영상은 **시간 측정**에 쓴다: 애니메이션 속도, 건설 시간, 다리 조각 생성 간격, 유닛 이동 속도, 공격 간격, Storm Power 증가 속도.
  60fps 이므로 프레임 번호 차이 × 약 16.7ms 로 잰다. 게임 틱(4단계)과 대조한다.

## 프레임 추출 도구 `tools/videoframes.py`

ffmpeg 로 필요한 구간만 디코딩하고, 게임 영역만 잘라 1024×768 로 줄인다 (`--native` 면 1440×1080 그대로).
결과는 git 에 넣지 않는 `extracted/videos/` 아래에 둔다.

```powershell
python tools/videoframes.py probe "playingVideos/Netstorm Islands at war - Dissolved Alliance.mp4"
python tools/videoframes.py frame "playingVideos/Netstorm Islands at war - Dissolved Alliance.mp4" 00:15:00 -o extracted/videos/da-15m.png
python tools/videoframes.py range "playingVideos/Netstorm Islands at war - Dissolved Alliance.mp4" 00:15:00 3 --fps 60 -o extracted/videos/da-15m
```

* `range` 결과의 n 번째 파일(1부터) 시각 = 시작 + (n−1)/fps 초.
* `cadence <영상> <시작> <초> [--max-y Y] [--top N]`: 카메라가 멈춘 프레임에서 자주 바뀌는 32×32 칸의 변화 간격 분포를 출력한다 (애니메이션 속도 측정).
  예: `python tools/videoframes.py cadence "playingVideos/Netstorm Islands at war - Early Missions.mp4" 600 12`

## 관찰 노트

* **[TEST01 녹화 화면 요소 대조·클론 반영 (2026-10-03)](test01-visuals-20261003.md)** — 같은 30FPS 녹화의 시작 화면·선택·배치 장면을 원본 스프라이트(1.25배 템플릿 매칭)와 exe 로 대조했다. 섬 테두리 색이 적용되지 않던 버그와 플레이어 색 규칙(색 번호 = 소유자 번호, `aiNColor` 덮어쓰기), 원본 방식 미니맵(1px = 2칸, 화면 중심을 따라 스크롤), 그림자(`shadow`·`flyershadow`), 가이저·워크샵·신전·풍선 애니메이션, `hotFootRatio` 기준점 이동과 시작 카메라, 브리핑의 글 사이 그림, 배치 미리보기와 커서 규칙(행 + 1 + `height`), 선택 괄호·체력 막대를 클론에 반영했다. 유닛의 플레이어 색, 연결 창, Crossbow 반짝임 등은 남겼다. 입력 기록 시각이 영상과 28.07초 어긋나는 점도 적었다.
* **[TEST01 추가 전투 판독·클론 수정 (2026-10-03)](test01-combat-20261003.md)** — 옮겨 온 30FPS 녹화에서 아이스 타워의 약 34.7초 재성장, 썬 바리케이트 선, 캐논 자세를 연속 판독하고 원본 타이머·방위·충전·탄속과 대조했다. 이동 진행량 보존·정지 자세, 고정 캐논 회전·사격, 재성장·무보상, 방어선 연결·흡수, 거리 비례 화면 이동을 반영했다. Assets 202·Core 310 검사와 클론 화면·우클릭 회전을 확인했다. 얼음 파편·정확한 피해·세부 회전·높이 예외는 남겼으며 원자료는 보존했다.

* **[원본 자동 분석 녹화: 캠페인 1-1 The War Begins! (2026-10-03)](auto-war-begins-20261003.md)** — `analyzeManager` 30FPS 자동 녹화(4,318프레임·236.9초, 실제 평균 18.2FPS). 사제·골렘 이동(선택 → 땅 좌클릭 → 선택 해제, 출발 지연 0.4~0.8초, 사제 약 1.8·골렘 약 2.0칸/초, 허공 클릭은 거부), 골렘 배치 연출(약 1.4초), 사제의 Sun Workshop 건설(약 10~11초, 창문 불), 워크샵 덱 등록(Rain Generator·Sun Cannon), ESC → Leave Mission → Main Menu를 시각표와 함께 정리했다. 1차 실행 녹화가 승인 프롬프트로 VS Code가 앞으로 나오며 중단된 사례와 파일 교체 거부 수정도 기록했다. 원자료는 Git 제외 `extracted/analyzeManager/20261003T052032571Z-e652ee405b0d/recording/`에 보존한다.
* **[녹화 원자료 최종 판독과 클론 반영 (2026-10-02)](record-play-final-20261002.md)** — 세 `record-play` 세션(1-1 전체·1-2 재현·UI 시연)을 지우기 전에 룬 마크(룬별 1.87~2.42초)·지키기 10.04초·제단 폭발 9.45초, 희생 음악 재시작(사제를 제단으로 보내는 명령), SP 숫자 따라가기, 낙하 궤적, 반복 효과음·명령 응답음, 시작 팁·View/Help 메뉴·우클릭 창·도움말 색을 판독해 클론에 반영했다. **세 세션의 원자료(영상·소리·입력)는 2026-10-02에 삭제했다.** 1초 간격 축소 프레임·시각표·버튼/키 사건 요약은 `extracted/record-play-archive-20261002/`에 남긴다. `playingVideos/`의 참고용 mp4는 그대로다.

* 네 번째 녹화 추가 판독(2026-10-02): [옵션 메뉴 동작·파란 마크의 소멸/복귀](../screens/options-menu.md), [도움말 11주제 한국어 정리](../gameplay/help-text-record-play-20261001.md), [내장 도움말 전체 영문 본문](../sources/in-game-help.md). 도움말의 중복 정의와 실제 녹화 목록을 구분하며, 원본의 2,727행 전체도 UTF-8로 보존했다.
* [ui-controls-record-play-20261001.md](ui-controls-record-play-20261001.md) — 네 번째 녹화, 2,563프레임·약 4분 16초. 미션 완주 대신 옵션·우클릭·생산 등록·미션 이탈·내장 도움말을 시연했다. [원본 조작키 표](../gameplay/input-controls.md)는 도움말 기재와 실제 Escape 입력을 구분한다. 지연 대응 빌드의 상대 훅 도착 대기는 최대 17.3ms였으며 포획·의식은 재현하지 않았다. 원자료는 2026-10-02에 삭제했다(축소 보관본: `extracted/record-play-archive-20261002/`).
* [master-of-whirligigs-record-play-20261001-2311.md](master-of-whirligigs-record-play-20261001-2311.md) — 캠페인 1-2 재플레이 6,318프레임·약 10분 36초. 포획 이전 작업장 생산·다리·전투·신전 파괴·사제 기절을 판독했다. 포획 뒤 클릭·커서 지연 보고와 후반 훅 도착 지연(상대 최대 약 1.05초), 녹화기의 입력 저장 분리 수정·미검증 범위를 기록했다. 의식 구간은 정상 타이밍 근거로 쓰지 않는다. **세 번째 녹화 원자료(영상·사운드·입력)는 2026-10-01 사용자가 삭제했다. 분석 문서와 추출 이미지·통계 등 산출물은 보존됐다.**
* [animation-timing.md](animation-timing.md) — 애니메이션 진행 속도: 가이저 증기 약 24Hz, 신전 회오리·피해 연기 12Hz. exe 의 "현재 시각 + 간격" 타이머와 `maxFPS = 75` 루프 양자화로 설명됨
* [the-war-begins-record-play-20260930.md](the-war-begins-record-play-20260930.md) — 사용자 직접 조작 10 FPS 녹화: 캠페인 1-1 시작, 지식 격자·설명창(호버·구성·시간 흐름), 적 신전 파괴 뒤 섬 테마·소유권 전환, 사제 기절·골렘 포획·제단 의식(다섯 룬)·희생 음악, 승리와 다음 미션 진입. 소리 판독 결과는 [music.md](../exe/music.md). 원본 녹화(`playingVideos/20260930T154921831Z-8bdcc06b6539/`)는 2026-10-02에 삭제했다(축소 보관본: `extracted/record-play-archive-20261002/`)
* [master-of-whirligigs-record-play-20261001.md](master-of-whirligigs-record-play-20261001.md) — `record-play` 10 FPS 녹화(캠페인 1-2). 다음 장면을 사용자가 일부러 재현했다.
  * 실제 전투
  * 운반 골렘의 다리 붕괴 낙하 → 사제 허공 기절 → 다리 재건 시 복귀
  * 내 사제의 허공 기절·복귀
  * 운반 골렘 판매·`Drop`
  * 의식 중 사제 이탈(룬 마크 전/후)
  * 의식 중 제단 판매
  * 완료 보상 +5,000 SP

  원본 녹화(`playingVideos/20261001T114152818Z-778e248ca3aa/`)는 2026-10-02에 삭제했다(축소 보관본: `extracted/record-play-archive-20261002/`)
* AV1 소프트웨어 디코딩은 느리므로 긴 구간을 한 번에 뽑지 않는다.

## YouTube 영상 바로 읽기 (2026-10-01)

YouTube 영상은 내려받지 않고 `analyzeManager`의 `youtube_*` 도구(MCP·CLI)로 지정 시각 프레임·짧은 구간만 읽는다. 재생기 광고가 섞이지 않는 원본 스트림을 쓰므로 영상 시각이 업로드 원본 시각과 같다. 사용법·광고 처리: [analyze-manager.md](../analyze-manager.md#youtube-영상-분석-원본-게임-실행-없음). 영상 목록은 AGENTS.md 와 LEFT_JOBS.md 5단계 표, 전용 채널 `https://www.youtube.com/@netstormcampaigns2591`(미션별 스피드런 다수).

## record-play 프레임·관찰표 도구 `tools/recordplay_frames.py` (2026-10-01)

`record-play`·`guide` 세션 폴더의 MJPEG AVI 조각을 FFmpeg 없이 읽는다. 조각마다 프레임 번호가 다시 시작하고 캡처가 멈춘 구간도 있으므로, 시각은 각 `.frames.csv`의 `sessionElapsedMs`로 계산한다(0 = 첫 프레임, `audiomatch.py`와 같은 기준).

- `frames <세션> -o <폴더> 660.5 g6596 …`: 경과 초 또는 `g`전역 프레임 번호의 원본 크기 PNG를 저장한다.
- `sheet <세션> -o <폴더> --step 50 [--start 초 --end 초] [--crop x,y,w,h]`: 시각을 적은 관찰표를 만든다. `--crop`이면 원본 픽셀 그대로 잘라 붙인다.

## 스프라이트 맞추기 도구 `tools/sprite_match.py` (2026-10-03)

녹화 프레임에서 오브젝트가 그리는 클러스터를 찾는다. `tools/shp.py export` 로 내보낸 `extracted/shapes/<번호>_<타입>/` 을 캡처 배율(기본 1.25)로 키워 불투명 픽셀의 평균 RGB 절대차를 구한다(작을수록 맞음).

```powershell
python tools/sprite_match.py playingVideos/<세션ID> --type geyser --anchor 940,521 --start 38 --end 39        # 프레임마다 가장 잘 맞는 클러스터
python tools/sprite_match.py playingVideos/<세션ID> --type rainBalloon --anchor 716,367 --times 37.5 --radius 20  # 기준점을 모를 때 주변을 넓게 찾는다
python tools/sprite_match.py playingVideos/<세션ID> --type thunderFactory --clusters B --locate --times 37.5      # 프레임 전체에서 찾는다
```

- 결과의 `(+dx,+dy)` 는 준 기준점에서 어긋난 캡처 픽셀이다. 타입별 `hotFootRatio` 이동([test01-visuals-20261003.md](test01-visuals-20261003.md) 4절)을 이 값으로 쟀다.
- 같은 이름 클러스터가 여러 개인 타입(windWalker)은 `이름#순번` 으로 구분한다. `--layer 1` 은 그림자 레이어다.
- `--locate` 는 생산 창 아이콘에 걸릴 수 있다. 큰 건물로 카메라 위치를 잡는 데만 쓴다.

## 소리 판독 도구 `tools/audiomatch.py` (2026-10-01)

`record-play`·`guide` 녹음(세션 폴더의 `audio-*.wav`·`*.start.txt`)을 이어 붙여 원본 소리와 FFT 정규화 상호상관으로 대조한다. 세션 폴더를 주면 시각은 `video-0001.frames.csv` 첫 프레임 기준 영상 경과 시각이다. YouTube `youtube_clip`처럼 단일 소리 포함 영상 파일을 주면 `--offset 초`를 더해 원본 영상 시각으로 출력한다. numpy 와 ffmpeg 가 필요하다.

```powershell
python tools/audiomatch.py level playingVideos/<세션ID>                       # 5초 구간별 dBFS
python tools/audiomatch.py music playingVideos/<세션ID> -o extracted/audio/music.csv   # 6초 조각별 곡·곡 안 위치 (+ 곡 구간 요약)
python tools/audiomatch.py sfx   playingVideos/<세션ID> -o extracted/audio/sfx.csv     # 원본 효과음 전체(상관 ≥ 0.55)
python tools/audiomatch.py sfx   playingVideos/<세션ID> --threshold 0.3 --sounds originals/sound/forWind2.WAV
python tools/audiomatch.py sfx   extracted/youtube/<ID>/clips/<구간>-a.mp4 --offset 800 --threshold 0.3 --sounds originals/sound/forWind2.WAV originals/sound/itIsDone2.wav
```

* 음악은 효과음·바람 소리와 섞여 상관값이 0.1~0.25로 낮다. 곡 판정은 **곡 안 위치가 조각 간격만큼 정확히 늘어나는 연속 구간**으로 한다(요약 출력).
* 내용이 같은 원본 파일 쌍(`openGump`/`openGump-1000`, `impact`/`thunderCannonImpact`, `influenceIcon`/`teleportEffect`)은 구분할 수 없다.
* YouTube 방송 영상에는 진행자 음성이 섞여 상관값이 낮아진다. `--threshold 0.2~0.3`으로 낮추고 `--sounds`로 효과음을 제한한다. 자동 검출은 후보를 고르는 도구이므로 화면·오디오를 함께 확인한다.
