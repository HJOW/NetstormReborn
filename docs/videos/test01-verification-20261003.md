# TEST01 화면 구현 후속 검증 (2026-10-03)

`LEFT_JOBS.md` 맨 위에 남겨진 [화면 요소 복원](test01-visuals-20261003.md)의 실행 검증을 진행했다. Windows 10 `HJOW-Athlon`에서 클론만 실행했으며 원본 게임은 실행하지 않았다. 원본 녹화·바이너리와 AGENTS.md는 수정하지 않았다.

## 소유자 색 복원

원본 `Colorsort.cpp`의 `0043be30`이 참조하는 타입 번호 전역 변수는 VA `0x541170`부터 연속으로 초기화돼 있다. 초기값은 70부터 시작하며 `TypeLoadOrder`와 같은 순서다. 따라서 `0x541214`는 `sunBalloon`, `0x54128c`는 `windBalloon`, `0x54127c`는 `windWalker`, `0x5412d0`은 `isle`이다. 이름 포인터 배열만 추적하던 앞 작업의 미확인을 해소했다.

새 `ObjectColorRemap`은 원본 지정 팔레트 인덱스만 바꾸고 나머지는 유지한다. 팔레트 번호의 실제 RGB는 배포 기본 팔레트에서 읽는다.

| 원본 경로 | 적용 타입 | 규칙 |
|---|---|---|
| `0043ba00` | bridge·bridgeConnector·priest·altar·bulf·sunWalker·rainWalker·sunFlyer·fenceShield·rainBalloon·rainFlyer·outpost | 228~237은 가장 밝은 후보, 238~245는 어두운 후보부터 밝은 후보까지. bridge의 빨강·연파랑은 이 어두운 띠를 각각 후보 4·3으로 고정 |
| `0043bbe0`, 명도 선택 `0043b5a0` | sunBalloon·windBalloon·windWalker | 각 타입에 지정된 픽셀을 소유자 색의 여덟 후보 중 가장 가까운 HSL 명도로 바꾼다. windBalloon의 명도 보정은 원본 double `-0.15000000596046448` |
| 기존 isle 변환 | isle·isleBig·edgeFarm·island·islandStalag | 이전 지면·받침 테두리 규칙 유지 |

색별 후보 표는 원본 VA `0x531a08`의 8×8 바이트다. 명도는 `(최대 RGB + 최소 RGB) / 510`이며 동률이면 먼저 나온 후보를 사용한다. 중립·범위 밖 색·변환이 없는 타입은 원래 색이다. 배포 팔레트의 여덟 소유자 색·다리 예외·풍선/Sail Skater 지정 색·중립·기존 지면 보존을 20개 테스트로 검사했다.

지도 저장 오브젝트, 이동 유닛, 새 배치 유닛, 공중 공격체, 낙하 그림, 배치 미리보기의 본체·겹침 그림에 적용했다. 타입에 따라 같은 셰이프도 다른 변환표를 사용하므로 텍스처 캐시 키에 타입 번호를 추가했다. 현재 소유자별 애니메이션 그림을 미리 준비한다. 그림자는 이전 음영 근사로 유지한다.

## 시작 지식 all 수정

실제 TEST01에서 아이스·썬더 캐논을 등록하려고 하니 `UnknownKnowledge`로 실패했다. `myTech=all`을 문자열 `all` 하나로 저장하고 개별 기술로 확장하지 않던 문제였다.

원본 `Player.cpp` `004914e0`은 대소문자를 무시하고 `all`을 인식하며 로딩된 타입 중 `group != NO_GROUP(10)`을 순서대로 넣는다. `MissionStart.ExpandKnowledge`로 사람·AI 시작 지식에 같은 처리를 적용했다. 배포 타입에서는 27개이며 효과 그림·미사일·장식은 포함하지 않는다. 기존 `techAllowed` 필터와 중복 이름 제거를 유지했다. 실제 TEST01의 저장 워크샵에 두 캐논을 등록하는 명령도 테스트한다.

## 사용자 확인: 캐논 설치 방향

2026-10-03 사용자 설명을 확정 규칙으로 기록했다.

- **썬 캐논:** 설치 방향을 정하지 않는다. 목표물이 바뀌어 공격 방향이 달라지면 스스로 회전한다.
- **아이스·썬더 캐논:** 설치 전에 마우스 우클릭으로 방향을 정한다. 설치 후에는 그 방향으로만 공격하며 수동·자동으로 회전하지 않는다.

클론의 기존 구분을 확인하고 회귀 테스트를 보강했다. 두 고정 캐논 모두 배치 명령의 방위를 적용하고, 썬 캐논은 같은 값을 무시한다. 첫 목표가 기절해 다른 방향의 목표로 바뀌는 상황에서 썬 캐논은 재발사 대기 중에도 방향을 바꾸고 두 고정 캐논은 옆 목표를 공격하지 않는다. 원본 썬 캐논의 접기·중간 회전 그림과 조준 지연은 별도 미복원 과제다.

## 실행 검증과 재현

```powershell
dotnet build -c Release
dotnet test -c Release --no-build
powershell -NoProfile -ExecutionPolicy Bypass -File tools/clone_ui_smoke.ps1 -OutputDirectory extracted/screens/test01-verify-20261003/final-ui
powershell -NoProfile -ExecutionPolicy Bypass -File tools/clone_help_options_smoke.ps1 -Mode Menu -OutputDirectory extracted/screens/test01-verify-20261003/final-help-menu
powershell -NoProfile -ExecutionPolicy Bypass -File tools/clone_help_options_smoke.ps1 -Mode Mission -OutputDirectory extracted/screens/test01-verify-20261003/final-help-mission
powershell -NoProfile -ExecutionPolicy Bypass -File tools/clone_help_options_smoke.ps1 -Mode Workshop -OutputDirectory extracted/screens/test01-verify-20261003/final-help-workshop
powershell -NoProfile -ExecutionPolicy Bypass -File tools/clone_move_smoke.ps1 -OutputDirectory extracted/screens/test01-verify-20261003/final-move
powershell -NoProfile -ExecutionPolicy Bypass -File tools/clone_test01_smoke.ps1 -OutputDirectory extracted/screens/test01-verify-20261003/test01-final
```

- Release 빌드: 경고 0·오류 0. 단위 테스트 Assets 255 + Core 371 = **626건 통과**, 건너뜀 0.
- 기존 UI 검사 26개 단언과 설정 저장 검사가 통과했다. 화면 전환·1-1/1-2 진입·전체화면 왕복을 확인했다. 도움말 Menu/Mission/Workshop 모두 통과했고 타이머 일시정지/재개도 확인했다. 사제 13개·골렘 9개 단언으로 그림 몸통 선택·배치·이동을 확인했다.
- 새 TEST01 검사는 1280×960 영어, 1024×768 한국어, 1280×720 영어, 1280×800 한국어에서 논리 높이 768을 사용한다. 각 조합 **24개 단언, 총 96개가 통과**했다. 사제·풍선·Sail Skater·워크샵·신전 선택, 저장 워크샵 II/III, F7 중립 테두리, 미니맵 클릭/끌기, Shift+1 저장/1 복귀, 배치 허용/불가 그림, 두 고정 캐논의 배치 전 회전을 확인했다. 미니맵은 입력 전후 카메라 좌표 변경과 저장 좌표 복귀를 단언한다.
- 로그·PNG는 Git 제외 `extracted/screens/test01-verify-20261003/`에 있다. 각 스크립트는 설정 폴더를 분리해 사용자 설정을 덮어쓰지 않는다.

원본 프레임은 `playingVideos/20261003T074745758Z-adc787856962/`의 36.0·37.5·103.5·120.5초를 `tools/recordplay_frames.py`로 추출했다. 녹화는 1280×960, 논리 화면 대비 배율 1.25다. 시작 클론 캡처와 37.5초 프레임의 카메라·건물 배치·생산 창·미니맵을 대조하고 선택·배치 화면의 괄호·체력 막대·원소/비용 표시도 살폈다. 영상의 `recordingElapsedMs`를 기준으로 한다.

## 성능 표본과 후속 과제

같은 TEST01·1280×960·논리 높이 768에서 `--perf`와 `wait 60; click-center 82,79; wait 600; quit;`을 사용해 PNG 저장 없는 표본을 측정했다. 로그와 설정은 `extracted/screens/test01-verify-20261003/perf-final/`에 있다.

| 설정 | 시작/일시 지연을 제외한 후반 표본 | 한계 |
|---|---|---|
| 기본 수직 동기 | 11.9~14.0초: 26.2~27.6FPS, CPU 그리기 평균 7.10~8.43ms | 이 환경에서 30FPS 유지도 아직 보장되지 않음 |
| `NETSTORM_NOVSYNC=1` | 10.9~13.9초: 58.9~60.1FPS, CPU 그리기 평균 6.85~10.13ms | 진단용 수직 동기 해제이며 60/120FPS 설정 구현과 구분 |

수직 동기 대기·GPU 시간은 CPU 그리기 평균에 포함되지 않는다. 텍스처 1,656개 예열은 약 479ms였고 최초 UI 그리기에 약 1초, 약 9.9초의 UI 구간에 361~438ms의 단발 지연이 있었다. UI 문자열/그림의 첫 사용 비용은 원인을 더 조사해야 한다. 이전 PNG 캡처를 포함한 큰 그리기 시간을 평상시 비용으로 사용하지 않았다. 프레임 상한 설정과 이 지연을 다음 우선 작업으로 남긴다.

## 남아 있는 차이

실행 검증 통과는 원본과 모든 픽셀이 일치한다는 의미가 아니다. 지면 타일과 가장자리 변형·그림자의 음영/그리기 순서·비례 글꼴·브리핑 크기, 에너지 방울과 반투명 배치 연출이 남아 있다. Crossbow의 V형 사거리, Wind/Rain 신전 애니메이션 간격, Sun 워크샵 창 점등, `CellCenterScreen` 위에 맞춘 방어선·탄 위치도 후속 과제다. 원본 시작 연결 창과 30/60/120FPS 설정, Linux GUI 검증은 이번 범위에서 구현하지 않았다.

관련 구현: [색 변환](../../dotnetpj/src/Netstorm.Assets/ObjectColorRemap.cs), [시작 지식](../../dotnetpj/src/Netstorm.Core/Rules/MissionStart.cs), [캐논 회귀 검사](../../dotnetpj/tests/Netstorm.Core.Tests/RecordedCombatTests.cs), [TEST01 실행 검사](../../tools/clone_test01_smoke.ps1).
