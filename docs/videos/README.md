# 원본 플레이 영상 자료

영상별 관찰 노트(`docs/videos/<이름>.md`)의 공통 전제와 프레임 추출 방법을 정리한다.
영상 목록과 YouTube 주소는 LEFT_JOBS.md 5단계 표에 있다.

## 로컬 영상 (`playingVideos/`, git 제외)

원본 게임을 최대 해상도 1024×768 **풀스크린**으로 두고 16:9 모니터에서 플레이하며 방송 중에 녹화했다 (AGENTS.md).

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
* AV1 소프트웨어 디코딩은 느리므로 긴 구간을 한 번에 뽑지 않는다.
