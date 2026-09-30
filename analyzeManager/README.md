# AI용 원본 게임 분석 도구

AI가 CLI 또는 로컬 stdio MCP로 원본 NetStorm의 복사본을 실행·조작하고, 화면 증거와 관찰 메모를 남기는 Windows용 도구다. 외부 AI가 다음 행동을 선택하며 도구 안에 별도 AI 모델은 포함하지 않는다.

사용자 직접 조작용 `guide --session ID --steps-file 안내.txt` 모드도 있다. 게임 옆에 단계 안내를 띄우고 사용자 입력·화면·기본 출력 장치 소리를 분할 저장한다. 기존 CLI/MCP 조작 방식은 그대로 사용할 수 있다. [사용법과 파일 형식](../docs/analyze-manager.md#사용자-직접-조작-녹화-모드).

**현재 상태:** Windows에서 실제 게임 실행·MCP PNG 전달과 사용자 직접 조작 녹화(튜토리얼 1·2 완료, 영상·소리·입력 기록)를 확인했다. Wine에서는 입력 전달·창 DC 캡처를 확인했다. 최신 캡처 변경 뒤 Windows의 게임 없는 Release 빌드·단위 테스트·MCP 기본 검사는 통과했다. 실제 녹화 결과와 남은 검증 범위는 [검증 상태](../docs/analyze-manager.md#사용자-직접-조작-녹화-모드)에 기록했다.

**실행 전 개발자 확인:** 일반 시스템에서 실제 게임 구동이 필요하면 개발자에게 목적과 필요성을 설명하고 명시적인 확인을 받아야 한다. CLI/MCP `start_session`, `mcp_smoke.py --live`, 검증용 직접 실행 모두 대상이다. **예외:** `AGENTS.md`에 지정된 시스템 — 시스템 1(IP `10.0.0.15`, 호스트명 `vm-debian-codex`, Linux/Wine), 시스템 2(IP `192.168.0.94`, 호스트명 `HJOW-Athlon`, Windows) — 에서는 확인 없이 실행할 수 있다. 또한 개발자가 **기존 게임 수동 컨트롤 방식으로 분석 진행을 직접 요청**한 경우 해당 작업 단계에서는 시스템과 무관하게 확인이 필요 없다. 목록 변경은 개발자에게 `AGENTS.md` 수정을 요청한다(AI는 `AGENTS.md`를 수정하지 않는다). 그 밖의 시스템에서는 기존 중단 지시의 재개도 필요하다. 게임 없는 빌드·정적 분석·단위 테스트·MCP 프로토콜 검사는 확인 대상이 아니다. 이 도구는 Windows용이다. Linux에서는 [linux-wine.sh](linux-wine.sh)로 Wine 실행 환경을 준비한다(게임 없는 검사·실제 입력 전달·캡처 확인. Wine에서는 게임 창 DC 복사로 캡처하며 증거의 `method`가 `wine-window-dc`로 남는다 — [문서](../docs/analyze-manager.md) "Linux(Wine)에서 사용").

- [사용법·저장 형식·검증 범위](../docs/analyze-manager.md)
- [최신 인수인계](../LEFT_JOBS.md)
- [MCP 설정 예시](examples/mcp-settings.json)

이 프로젝트는 Windows 전용이므로 게임 솔루션과 별도로 빌드한다.

```powershell
dotnet build analyzeManager/AnalyzeManager.csproj -c Release
```

`bin/`, `obj/` 및 실행 기록을 담는 `extracted/analyzeManager/`는 기존 `.gitignore`에 따라 Git에서 제외된다.
