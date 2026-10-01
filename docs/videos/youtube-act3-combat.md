# YouTube 캠페인 3-1~3-5 자료: 전투 1차 관찰

2026-10-01 사용자가 내려받아 추가한 `playingVideos/[Youtube] 3-1 to 3-5.mp4`를 읽었다.
출처는 사용자 지정 [YouTube 영상](https://www.youtube.com/watch?v=0p7VvzSxTAY)이다.
웹 페이지 조회는 실패했지만 **로컬 영상 파일을 직접 디코딩해** 아래 내용을 확인했다.

## 파일과 관찰 범위

- ffprobe: **6,093.5초(1:41:33.5)**, 854,625,381바이트, H.264 1280×720, 30fps.
- ffprobe가 나열한 스트림은 영상 하나다. **이 파일에는 오디오 트랙이 없다.** 소리로 발사·충돌·음악을 측정하지 않았다.
- 화면은 16:9 폭을 쓰며, 기존 방송 녹화의 좌우 240px 잘라내기 규칙을 적용하면 안 된다.
  화면 비율만으로 영상 속 게임 패치 버전이나 맵 좌표의 픽셀 배율을 단정하지 않는다.
- 전체를 연속 재생한 것이 아니다. **00:00~1:40:00을 2분 간격 51장**으로 훑고,
  **08:00~08:05.5는 0.5초 간격 12장**, **1:20:00~1:20:01.1은 0.1초 간격 12장**을 추가로 읽었다.
- 미션별 시작·종료 시각, 다섯 미션의 성공 여부, 다른 구간의 전투 규칙은 아직 전수 확인하지 않았다.
- 숫자 피해량이나 원본 게임 시간 배속은 이 표본만으로 측정할 수 없다. 아래 시각은 **파일 재생 시각**이다.

## 확인한 장면

| 파일 시각 | 직접 보이는 내용 | 해석·클론 반영 |
|---|---|---|
| 00:02:00~00:12:00의 2분 표본 | 적 갈색 섬 주변에 다리·포대·방어 구조물이 있고, 후반 표본에는 녹색 지면으로 바뀐 모습이 나온다 | 구간을 건너뛴 표본이므로 신전 파괴 정확한 시각은 확정하지 않음. 신전 생존 여부로 지면 원소·소유권을 바꾸는 기존 관찰과 모순 없음 |
| 00:08:00~00:08:05.5 | 오른쪽의 푸른 구조물 열, 적 섬 위의 원반형 유닛 및 발사 섬광, 상단을 지나오는 공중 이동체. 카메라도 중간에 움직임 | 건물형 공격과 이동체를 구분해야 함. 이 표본으로 발사 주기·탄속·비행 속도를 수치화하지 않음 |
| 00:18:00~00:26:00의 2분 표본 | 다리 가지 위의 포대가 적 섬 둘레를 향하고, 섬 위·다리 위에 서로 다른 방어 유닛이 밀집 | 포대 목표 탐색·생존 체력·파괴 효과를 실제 맵에 연결하는 작업 범위에 반영. 차폐 타입별 효과는 미확정 |
| 00:44:00~00:52:00의 2분 표본 | 여러 섬 사이의 포대 열과 전투 흔적, 후반 표본에서 섬 지면이 녹색으로 바뀜 | 장면 사이 인과관계·정확한 파괴 시각은 후속 연속 프레임 분석 필요 |
| **01:20:00.0~01:20:01.1** | 왼쪽의 어두운 층형 탑에서 오른쪽 목표 방향으로 흰색·청록색 꺾인 선이 이어진다. 00.0~00.2에 보이고 00.3~00.9에는 없으며 01.0~01.1에 다시 보인다 | 추출한 `078_thunderArcher/A00_L0.png`와 탑 형태를 대조해 **Vander Tower**로 식별. 같은 시간 표본에 나타나는 번개와 이동 탄을 구분해 구현 |

Vander Tower의 `.type`에는 `delayBetweenShots = 1.0`, `hpPerSec = 35`, `range = 15`가 있다.
약 1초 차이로 보이는 번개 표본과 모순되지 않지만 정확한 시작 프레임 측정은 아니다.
**번개 표시 0.2초·피해 다음 1틱**은 구현을 진행하기 위한 임시 계약이다. 영상이 내부 피해 계산을 증명하지는 않는다.
자세한 확정/추정 구분은 [전투 구현 문서](../gameplay/combat.md)를 따른다.

## 재현

`tools/video_contact.py`는 각 시각을 직접 탐색해 시각 라벨이 붙은 관찰표를 만든다.
기존 방송용 `videoframes.py`와 달리 기본적으로 화면을 자르지 않는다. `--crop 폭:높이:x:y`로 특정 영역을 볼 수 있다.
FFmpeg와 Pillow가 필요하다.

```powershell
python tools/video_contact.py "playingVideos/[Youtube] 3-1 to 3-5.mp4" --start 0 --step 120 --count 18 -o extracted/youtube-act3-overview-1.png
python tools/video_contact.py "playingVideos/[Youtube] 3-1 to 3-5.mp4" --start 2160 --step 120 --count 18 -o extracted/youtube-act3-overview-2.png
python tools/video_contact.py "playingVideos/[Youtube] 3-1 to 3-5.mp4" --start 4320 --step 120 --count 15 -o extracted/youtube-act3-overview-3.png
python tools/video_contact.py "playingVideos/[Youtube] 3-1 to 3-5.mp4" --start 480 --step 0.5 --count 12 --width 640 -o extracted/youtube-act3-battle-0800.png
python tools/video_contact.py "playingVideos/[Youtube] 3-1 to 3-5.mp4" --start 4800 --step 0.1 --count 12 --width 640 -o extracted/youtube-act3-battle-8000.png
```

산출물과 원본 영상은 Git에서 제외된다. 다음은 원본 크기의 짧은 연속 프레임으로 Vander 발사 시작점을 잡고,
공중 기지의 출격·귀환, 블로커 차폐·반사, 미션 성공 전 사제 운반을 별도로 기록한다.
