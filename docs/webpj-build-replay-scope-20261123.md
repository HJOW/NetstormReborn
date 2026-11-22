# 건설 기록 재생(RawBuildReplay·ClientBuildReplay)의 웹 범위 판정 (WB04-E1242)

E1223 읽기 전용 감사가 두 행의 남은 일 — `.rec` 파일 판독(`ReadBuildReplay`의 fscanf)·`MakeHooks`·재생 Process·`StartLatest`·Ctrl+F7 키 분배·`SelectBuildReplayObject` — 을 "개발자 전용이라 웹 제외로 문서화할 수 있다"고 남겼다. 소스로 도달 경로를 확인해 **웹 범위 밖으로 닫는다.** 이미 이식한 Run/Label/Notify 본체와 장기 전역·context·라벨 부모는 소스 계약 시험으로 남기고 앱에는 더 연결하지 않는다.

## 근거

1. **재생을 시작하는 입력은 개발자 관문 뒤에만 있다.** `DispatchBuildReplayKey`(`ClientBuildReplay.cpp:73~77`)는 `F7` 키(`0x760000`)가 눌림이고 정지 중이 아니며 **`specialAuxiliary`(원문 `0055a484`)가 0이 아니고 Ctrl이 눌렸고** 메타맵이 아닐 때만 `StartLatestBuildReplay`(Shift면 NextStage 명령)를 부른다. `UberGump.cpp:1727`은 이 분배를 일반 F7(배너 토글)보다 먼저 시도하되 관문에서 `false`면 일반 F7로 넘어간다.
2. **`specialAuxiliary`는 일반 사용자 경로로 켜지지 않는다.** 시작할 때 `ClientMain.cpp:744`(설정 해석)가 `player.special`과 함께 0으로 쓰며, 값을 바꾸는 쪽은 `WriteDebugWord(0x55a484)`뿐이고 이 쓰기는 AI 디버그 명령 분배(`UberGump.cpp:1502~1507`, `IsAiDebugCommand`)·개발 세션 디버그 상태(`RawCampaignSession.cpp:6414`)의 `writeDebug` 장치로만 연결된다. 웹에는 개발 콘솔·AI 디버그 입력이 없다(E1163 분류).
3. **일반 플레이의 건설 확정은 재생과 무관하게 동작한다.** 실제 건설 확정의 `recorderActive`(`CurrentBuildReplayIsLabel`)는 재생 중인 `playing`이 있을 때만 참이고(`RawCampaignSession.cpp:2745`), 재생이 시작되지 않으면 항상 거짓이라 기록 알림(`NotifyCurrentBuildReplay`)도 호출되지 않는다. 웹 `core/client-build-replay.js`의 `playing`은 항상 0이다.
4. **`.rec` 재생 파일은 개발 도구의 산출물이다.** `FILE/최신 rec`은 원본 개발 과정에서 건설 기록을 저장하는 파일이며 배포 자산에 없다(`originals` 설치 디렉터리와 `assets`에 `.rec` 파일이 없음을 확인했다).

## 결론

- `RawBuildReplay.cpp`·`ClientBuildReplay.cpp` 두 행은 이식 대상 범위(Run/Label/Notify 본체·전역·context·두 현재 라벨 부모)를 마쳤으므로 상태를 partial → port 로 닫고 비고에 개발자 전용 제외 근거를 더했다. 인계 6단계 '건설·생산·경제'의 남은 일에서 `RawBuildReplay·ClientBuildReplay의 FILE/최신 rec·Host·F7/Ctrl+F6`을 지웠다.
- 개발자 입력(Ctrl+F7 재생·AI 디버그)을 웹에 도입하게 되면 그때 `ReadBuildReplay`·`MakeHooks`·재생 Process 연결·`StartLatest`·키 분배를 함께 옮긴다.
- 기준선 뒤 cpppj 변경 없음(`RawBuildReplay.cpp`·`ClientBuildReplay.cpp`). 이 단위는 코드를 바꾸지 않았고 cpppj 소스를 읽어 판정했다.

## 대응표 정리

두 행(`[rebaseline-adapt]`)의 남음이 위 판정으로 사라져 partial 단위가 32 → 30, 줄 수가 20,635 → 20,436이 됐다(`python -X utf8 tools/port_inventory.py`).
