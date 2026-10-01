# YouTube 도구 활용: 캠페인 1-1~4의 Whirligig 관찰

2026-10-01. AGENTS.md에 있는 [캠페인 1-1~4 영상](https://www.youtube.com/watch?v=AMEzorbjQYQ)을
`analyzeManager`의 `youtube_probe`·`youtube_frames`·`youtube_clip`·`youtube_note`로 읽었다.
**영상 전체 시청이나 모든 링크의 전수 분석은 아니다.** 원본 게임 실행 없이 진행했다.

## 조회·표본

- 제목: `넷스톰 Netstorm Islands at war 1-1~4 스테이지 2025.06.03`, 채널 `Mr. Heo 의 게임 기록소`.
- 메타데이터: 3,318초, 1920×1080·60fps. 챕터·SponsorBlock 구간 없음.
- 전체 구간 탐색: **300초부터 240초 간격 12장**(05:00, 09:00, …, 49:00), 640×360·30fps 스트림.
  스트림 길이 3317.466667초는 메타데이터와 도구 허용 오차 안에서 일치했다.
- 상세 표본: **13:00~13:20**, 1280×720·60fps 클립(1,521,805바이트).
  여기서 0~18초를 2초 간격 10장으로 추출했다. 클립 시각 `t`는 원본 `780+t`초다.
- 도구 기록: `extracted/youtube/AMEzorbjQYQ/{info.json,frames.jsonl,notes.jsonl,report.md}`.
  구간: `clips/000780.000-000800.000-h720.mp4`, 관찰표: `extracted/whirligig-reference.png` (모두 Git 제외).

## 관찰과 적용 경계

13분 표본에서 다리 끝의 고정 구조물과 별개로 흰 원반형 공격체가 섬 위를 이동한다.
워크샵·발전기 주변에는 타격 섬광이 보인다. 본체가 지상 건물·다리와 별도 높이/그림자로 표현된다.
13:08~13:10은 카메라가 위 섬을 보고, 13:12 이후 다시 아래 섬을 보므로 화면 변위로 속도를 계산하면 안 된다.
여러 비행체가 겹치므로 특정 기지와 특정 비행체의 귀환·재생성 대응도 이 표본으로 확정하지 않았다.

Whirligig의 **매분 보급·수송 공격 제외·목표당 최대 3대**는 화면만으로 추측하지 않고 원본
`d/help.english`로 확인했다. 수치·출발점 사거리는 `.type`과 패치 기록을 대조했다.
이번 클론의 생성 5초·보급 3초·접근 거리·피해 타이밍 등 미확정 값은
[비행체 구현 계약](../gameplay/flyers.md)에 별도로 적었다.

13:00 프레임의 SHA-256:
`88eb74273de81b52cc7e4284dec9aeafe28d8c7098104f715bcc36e5fb18d26c`.
`youtube_note`로 이 해시와 관찰/한계 메모를 연결해 도구 캐시에 저장했다.

## 재현

PowerShell에서 네이티브 실행 파일에 JSON을 직접 넘기면 따옴표가 소실될 수 있어 `--args-file`을 사용했다.
인자 파일에 다음 JSON을 순서대로 넣어 같은 도구를 호출한다.

```json
{"url":"https://www.youtube.com/watch?v=AMEzorbjQYQ"}
```

```powershell
& analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe call youtube_probe --args-file extracted/youtube-request.json
```

프레임 도구 인자:

```json
{"videoId":"AMEzorbjQYQ","start":300,"step":240,"count":12,"maxHeight":360,"columns":3,"cellWidth":480}
```

구간 도구 인자(`start`·`end`는 초 단위 숫자):

```json
{"videoId":"AMEzorbjQYQ","start":780,"end":800,"maxHeight":720}
```

각각 `call youtube_frames`, `call youtube_clip`으로 호출한다. 저장한 클립의 관찰표:

```powershell
python tools/video_contact.py extracted/youtube/AMEzorbjQYQ/clips/000780.000-000800.000-h720.mp4 --start 0 --step 2 --count 10 --width 600 --columns 2 -o extracted/whirligig-reference.png
```
