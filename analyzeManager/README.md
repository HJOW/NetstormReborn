# AI용 원본 게임 분석 도구

AI가 CLI 또는 로컬 stdio MCP로 원본 NetStorm의 복사본을 실행·조작하고, 화면 증거와 관찰 메모를 남기는 Windows용 도구다. 외부 AI가 다음 행동을 선택하며 도구 안에 별도 AI 모델은 포함하지 않는다.

**현재 상태: 구현 및 단위 테스트 완료, 실제 입력·MCP 통합 검증은 중단된 상태.** 2026-09-29 사용자가 다른 작업 때문에 게임 실행을 중단하도록 요청했다. 사용자가 재개를 명시하기 전에는 게임 실행이나 `mcp_smoke.py --live`를 수행하지 않는다.

**실행 전 개발자 확인:** 실제 게임 구동이 필요하면 개발자에게 목적과 필요성을 설명하고 명시적인 확인을 받아야 한다. CLI/MCP `start_session`, `mcp_smoke.py --live`, 검증용 직접 실행 모두 대상이다. **예외:** IP `10.0.0.15` 및 호스트명 `vm-debian-codex`에 해당하는 시스템에서는 개발자 확인을 받지 않아도 된다(`AGENTS.md`). 이 예외는 확인 절차에만 적용되며 위의 현재 실행 중단 지시는 유지된다. 중단 해제 후에도 일반 시스템의 확인 규칙은 적용된다. 게임 없는 빌드·정적 분석·단위 테스트·MCP 프로토콜 검사는 확인 대상이 아니다.

- [사용법·저장 형식·검증 범위](../docs/analyze-manager.md)
- [최신 인수인계](../LEFT_JOBS.md)
- [MCP 설정 예시](examples/mcp-settings.json)

이 프로젝트는 Windows 전용이므로 게임 솔루션과 별도로 빌드한다.

```powershell
dotnet build analyzeManager/AnalyzeManager.csproj -c Release
```

`bin/`, `obj/` 및 실행 기록을 담는 `extracted/analyzeManager/`는 기존 `.gitignore`에 따라 Git에서 제외된다.
