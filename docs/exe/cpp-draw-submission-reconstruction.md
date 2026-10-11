# 공통 표시 목록 병합·종류 분배 복원

2026-10-11, `HJOW-Athlon`. 원본 `004994b0` / CD·추가판 `004575e0`의 **유효 dirty 사각형 하나 안에서 수행하는 목록 병합과 kind 0~4 분배**를 복원했다. 실제 건설 표시 수집/Draw를 같은 경로에 연결했다. 수집기 내부와 전체 Client/GameWorld 프레임을 완료한 것은 아니다.

AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 먼저 확인했다. 착수 시 깨끗한 트리였으며 현재 PC와 마지막 디컴파일 PC가 같았다. `drawsubmission` **6/9/9개**를 읽기 전용 Ghidra로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본 게임/복사본·analyzeManager·클론 창은 실행하지 않았다.

## 복원한 흐름

`DrawSubmission`은 다음 순서로 16바이트 항목을 같은 작업 배열에 모은다.

1. UI/Gump: 화면 사각형과 전체 남은 용량을 전달한다.
2. dirty flags의 `4`가 켜졌으면 UI 목록만 반환한다.
3. 건설 등 등록 표시: 클립된 가시 월드 영역과 남은 용량을 전달한다.
4. 묶음: 가시 영역/호출자가 공급한 판본별 행 제한을 전달하고 배경 덮음 여부를 받는다. 묶음의 **첫 항목 x/y/signed WORD 깊이**, 묶음 헤더 토큰, `kind=1`을 쓴다. 작업 칸의 **tail은 보존**한다. 후속 `TileGroupCollection` 분석에서 이 인수는 시각이 아니라 각 행의 `count<drawListSize` assert 제한임을 확인해 `groupRowLimit`으로 바로잡았다.
5. 일반 자산/효과: 가시 영역과 남은 용량을 전달한다.
6. 미리보기: **클립 전 월드 영역**과 남은 용량을 전달한다.

원본 공통 배열은 **5,000항목**이다. 작은 호스트 작업 버퍼에서는 그 크기로 제한한다. 각 일반 수집기는 실제 남은 span/개수를 받고 용량 0에서도 호출된다. 원본 묶음 복사는 용량을 확인하지 않지만 호스트 복원은 넘치는 묶음을 거부한다. 음수/범위 밖 반환, 빈 묶음 저장소, 없는 개수 포인터도 예외로 거부한다. 앞 단계의 기록은 남을 수 있다.

정렬은 앞 단계에서 복원한 판본별 CRT `SortDrawEntries`를 **병합된 상위 목록에 한 번** 적용한다. 묶음 내부는 정렬하지 않는다.

| kind | 원본 제출 | 복원 경계 |
| --- | --- | --- |
| 0 | 표시 항목 자체를 자산 그리기로 전달 | `sprite(entry)` |
| 1 | 묶음 숨김 전역이 0일 때 내부 항목 순회 | `group(token)` → 내부 항목마다 `sprite(entry)` |
| 2 | UI/Gump 포인터 | `ui(token)` |
| 3 | 등록 표시 프로세스 포인터 | `construction(token)` |
| 4 | 미리보기 항목 전체, tail 포함 | `preview(entry)` |
| 그 밖 | switch의 그리기만 건너뜀 | 후처리는 그대로 수행 |

묶음은 최초 배열 주소를 유지하고 **현재 signed 개수를 매 차례 다시 읽는다**. 그리기 중 개수가 4→1로 줄거나 1→4로 늘 때도 원본 순회와 일치한다. 0/음수면 내부를 그리지 않는다. 내부 항목의 kind를 재분배하지 않고 항상 자산 그리기에 전달한다. CD의 별도 `00456900`도 같은 동작이다.

묶음 숨김/debug 전역은 현재 값을 읽는다. 앞선 UI 그리기가 값을 바꾸면 이후 항목에 반영된다. debug 후처리는 **상위 항목마다 한 번**이며 묶음 내부마다가 아니다. 알 수 없는 kind에도 실행된다. 호스트 `afterEach`는 원본 debug 사각형 그리기/클립 복구의 연결 경계이며 실제 장치 구현은 후속이다.

## 실제 건설 연결

`DrawVisibleScene`은 다른 종류의 수집/출력 경계를 받아 다음 몸체를 실제로 연결한다.

- 건설 수집: 같은 풀/등록 목록의 `ConstructionVisualSystem::Collect`.
- 정렬: 같은 풀의 판본별 CRT.
- kind 3: `DrawToken` → 살아 있는 실제 `ConstructionVisualProcess::Draw` → `RawConstructionFrame::Draw`.

기존 `DrawVisibleConstruction`도 이 공통 경로를 사용한다. 원본의 UI→건설→묶음→자산→미리보기 병합 순서와 정렬 끔을 유지한다. 수집 뒤 앞선 종류의 Draw가 건설 객체를 삭제해도 이전 토큰은 건너뛰며 재사용된 슬롯의 새 객체를 이전 목록에 섞지 않는다.

새 통합 검사는 실제 두 건설 프로세스/출력 버퍼를 서로 다른 깊이의 UI/묶음/자산/미리보기와 함께 제출한다. 묶음 내부는 깊이 순으로 재정렬하지 않으며 UI 전용 dirty에서는 실제 건설 Draw를 호출하지 않는다. **GUI가 자동으로 이 혼합 경로를 호출하는 프레임 연결은 아직 없다.**

## 독립 기계어 근거

`tools/decomp_drawsubmission_oracle.py`는 세 PE 각각 **322개**, 새 **966개**, 두 x87 정밀도에서 **1,932회 상위 Renderer 정상 반환**을 관찰한다. 원본 독립 관찰 누적은 **505,086개**다. 실행은 새 Ghidra 함수 범위로 제한하며 보존 레지스터/스택/x87/SEH·배열/미사용/보호 바이트·허용 쓰기를 검사한다.

실제 실행하는 부분은 상위 Renderer의 선택한 흐름, 좌표 환산/클립, 병합 쓰기, 판본별 CRT, switch/묶음 순회다. **수집기의 완성 배열/묶음/배경 덮음 응답과 그림/장치 내부**는 명시 경계다. 묶음 개수 변경과 UI의 숨김/debug 전역 변경도 해당 그림 경계의 입력 응답이다. 기대 병합/정렬/분배 순서는 계산하지 않는다.

입력은 mode 0/128, 유효 dirty 한 개, 구름/모드 전환/강제 갱신/격자 끔으로 제한한다. 실제 루트의 정상 반환은 검증하지만 전체 함수의 모든 분기를 복원했다고 주장하지 않는다. C++ 경계에 공급하는 두 월드 사각형과 판본별 묶음 행 제한은 **실제 루트가 계산한 인자 관찰**에서 읽는다. C++ 좌표 환산/제한 계산을 새로 검증하는 fixture가 아니다.

관찰은 빈/단일/완전 동률/혼합 kind/극단 signed 깊이/무작위/미리보기 tail 2, 개수 증가/감소/0/음수, dirty 4/정렬/숨김/debug 교차, 5,000항목 경계를 포함한다. 저장 fixture는 네 수집기와 묶음의 완성 입력, 병합 전후 전체 바이트, 수집 포인터 위치/용량/좌표, 모든 분배 인자/순서, 장치 경계 이름, 진입/경계 횟수, 보호 바이트를 저장한다.

저장 감사는 PE/도구/내보내기/fixture SHA, 입력 계획/전체 입력, 배열 폭/용량/보호, 실제 함수 진입/상위 루트 반환/명시 경계 집계를 검사한다. 원본 게임/OS/assert는 0이다.

## 검증

새 C++ **8개**: 세 PE 관찰 재생, 잘못된 반환/묶음 초과, 분배 계약/알 수 없는 종류, 작은/빈/5,000 버퍼, 실제 혼합 건설, 앞선 종류의 Draw 중 건설 삭제/슬롯 재사용. 집중 검사 **8개·실패 0**. Release 경고/오류 **0**, CTest 내부 **711개·실패 0**(151.17초·전체 151.21초), 근거 감사 **108종 모두 통과**. 새 변이 **8개 모두 검출**, 미검출/건너뜀 **0개**(`--focused`, 변이 전 사본 전체 검사 통과).

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name drawsubmission
python -X utf8 tools/decomp_drawsubmission_oracle.py --smoke
python -X utf8 tools/decomp_drawsubmission_oracle.py
python -X utf8 tools/decomp_drawsubmission_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
cpppj/build/bin/Release/netstorm_tests.exe --filter DrawSubmission
ctest --test-dir cpppj/build -C Release --output-on-failure
```

새 변이 8종은 UI 전용 비트, 미리보기 영역, 묶음 첫 키/tail/현재 개수/숨김, 알 수 없는 kind의 debug, 실제 건설 분배 누락이다. 별도 `extracted/mutation-work` 사본에서만 적용하고 원래 바이트로 복구한다. 로그는 `extracted/drawsubmission-{export,smoke,oracle,build,focused,ctest,audits,mutations}.log`다.

AGENTS.md·보호 원본·dotnetpj·dotnet 인계는 변경하지 않았으며 커밋/푸시하지 않았다.

## 다음 작업

1. **UI/Gump 수집·화면 위치/경계는 후속 [GumpCollection 단계](cpp-gump-collection-reconstruction.md)에서 복원했다.** 묶음 `00433410`, 일반 자산/효과 `00433660`, 미리보기 `00446060`의 수집기 내부는 남았다. 실제 Gump 생성/Draw·구름/배경·dirty 순회·잠금/출력·Client/GameWorld 프레임 호출 순서·클릭 선택을 연결한다.
2. 건설의 실제 SHP 캡처·공유 팔레트/현재 색 표·dirty·화면/장치 출력을 실제 프레임에 연결한다.
3. 타입 준비 소리(+0x90)·부모 공간/사제 contained·하위 경로/자원·집합/지연 분배·건설 GUI를 진행한다. **게임 내 건설과 미션 완주는 아직 불가능하다.**
