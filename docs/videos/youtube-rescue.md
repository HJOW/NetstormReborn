# 캠페인 3-4 Enemy Territory: 프리스트 구조 관찰

> 영상: [Netstorm Enemy Territory speedrun 2m10s](https://www.youtube.com/watch?v=t2g9fASt4do), 2026-10-01 분석. 업로드 길이 3분 5초, 1920×1080·30fps. 원본 게임을 실행하지 않았다.

Linux `youtube_frames`로 2:42~2:46 구간을 1920×1080으로 추출했다. 아래 시각은 업로드 영상 기준이다. 2:45.5와 2:46 프레임 사이의 드롭 입력 시각은 보이지 않으므로 구출 시점을 그 0.5초 범위로만 좁힌다.

## 관찰과 미션 스크립트

| 영상 시각 | 관찰 | 의미와 한계 |
|---:|---|---|
| 2:42.0 | 선택된 Sail Skater 메뉴에 `Class: Ground Transport`, `Carrying: High Priest`, `Drop High Priest`가 보임 | 프리스트를 지상 수송 유닛으로 운반 중 |
| 2:45.5 | 같은 유닛 메뉴에 `Carrying: High Priest`가 계속 보임 | 이 프레임 시점까지는 운반 상태 |
| 2:46.0 | `Success! You have brought the traitor to your island!` 안내와 `Leave Missions`, `Next Mission` 버튼이 보임 | 프리스트가 플레이어 섬에 내려진 뒤 성공 판정이 표시됨. 드롭 입력 자체는 2:45.5 이후, 2:46.0 이전 |

설치본 `extracted/tarc/d/enemyterritory.english`의 미션 스크립트는 다음 조건을 명시한다.

- `allowAnyCapture = 1`: 이 미션에서는 포획 대상이 기절했는지와 무관하게 포획을 허용한다.
- `[Ai2PriestCaptured]`: 포획 직후 `Saved!` 안내에서 플레이어 섬에 내려 안전하게 데려오라고 지시한다.
- `[Succeeded][Ai2PriestSaved]`: `You have brought the traitor to your island!` 성공 문구를 표시하고 미션 완료로 기록한다.

영상에서 `Saved!` 창이 나온 정확한 시각은 이번 표본으로 확인하지 않았다. Core 구현의 미션별 `allowAnyCapture` 예외와 사제 드롭 시 구출 이벤트는 이 스크립트 조건에 맞는다. 이는 한 구조 미션의 일치 근거이며, 모든 캠페인에서 기절 전 포획이 가능한 뜻은 아니다. 희생 의식과 이벤트 구현 조건은 [희생 의식 문서](../gameplay/sacrifice.md)에 정리했다.

## 프레임 근거

`youtube_note`로 아래 세 메모를 영상 프레임 SHA-256에 연결했다. 메모와 원본 해시 인덱스는 Git 제외 경로 `extracted/youtube/t2g9fASt4do/notes.jsonl` 및 `report.md`에 있다.

| 시각 | 메모 | 프레임 SHA-256 |
|---:|---|---|
| 2:42.0 | Sail Skater가 High Priest를 운반 | `bfd1265570f29ccb86ed58aed37b9a26f43dea1844904ffc0713909851b8c54e` |
| 2:45.5 | 여전히 High Priest 운반 중 | `e3b269b9f4e63c5328b20e6fad14325c1430748a12dc91c9b0245e022f1747d5` |
| 2:46.0 | 플레이어 섬에 traitor를 데려왔다는 성공 창 | `eb97361ad96404092be1a44c6d746becacdec4e2356dc64e5684dddf5c7fb630` |

프레임은 화면 문구와 운반 상태를 확인하는 증거다. 드롭 시점은 두 연속 프레임 사이로만 한정되며, 영상 속 조작 이벤트나 원본 실행 로그를 확인한 것은 아니다.
