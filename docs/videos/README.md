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

* [animation-timing.md](animation-timing.md) — 애니메이션 진행 속도: 가이저 증기 약 24Hz, 신전 회오리·피해 연기 12Hz. exe 의 "현재 시각 + 간격" 타이머와 `maxFPS = 75` 루프 양자화로 설명됨
* [the-war-begins-record-play-20260930.md](the-war-begins-record-play-20260930.md) — 사용자 직접 조작 10 FPS 녹화: 캠페인 1-1 시작, 지식 격자·설명창(호버·구성·시간 흐름), 적 신전 파괴 뒤 섬 테마·소유권 전환, 사제 기절·골렘 포획·제단 의식(다섯 룬)·희생 음악, 승리와 다음 미션 진입. 소리 판독 결과는 [music.md](../exe/music.md). 원본 녹화는 `playingVideos/20260930T154921831Z-8bdcc06b6539/`에 있으며 Git에서 제외됨
* AV1 소프트웨어 디코딩은 느리므로 긴 구간을 한 번에 뽑지 않는다.

## YouTube 영상 바로 읽기 (2026-10-01)

YouTube 영상은 내려받지 않고 `analyzeManager`의 `youtube_*` 도구(MCP·CLI)로 지정 시각 프레임·짧은 구간만 읽는다. 재생기 광고가 섞이지 않는 원본 스트림을 쓰므로 영상 시각이 업로드 원본 시각과 같다. 사용법·광고 처리: [analyze-manager.md](../analyze-manager.md#youtube-영상-분석-원본-게임-실행-없음). 영상 목록은 AGENTS.md 와 LEFT_JOBS.md 5단계 표, 전용 채널 `https://www.youtube.com/@netstormcampaigns2591`(미션별 스피드런 다수).

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
