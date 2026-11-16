# 공통 1317 — 옵션 없는 원본 관찰 사본 준비

2026-11-17, HJOW-Athlon. `SessionStore.CreateAsync`가 정상 최초 설치에서 없는 `d/options.cfg`를 필수 파일로 열어 `start_session` 준비에 실패하던 문제를 해결했다. 원본 보호 폴더에는 쓰지 않는다.

관찰 사본에 `options.cfg`가 없을 때만 빈 `ConfigFile`에서 시작해 기존 관찰용 네 키(`InstallDir`, `startInFullScreen=0`, `SCREENW=1024`, `SCREENH=768`)를 만든다. 이 화면 크기/창 모드는 도구 조건이며 게임의 최초 기본값이라는 뜻이 아니다. `setup.cfg`는 계속 필수다. 기존 옵션이 있으면 이전처럼 그 내용을 보존하면서 네 키만 바꾼다. 없는 옵션을 setup 복사로 채우거나 welcome·완료·음량 값을 강제하지 않는다.

새 단위 검사는 가짜 원본 자료만 만들고 게임을 실행하지 않는다. 옵션 부재에서 준비 성공·원본 옵션 부재 유지·원본 setup 바이트 불변·생성 옵션의 네 키 한정·setup의 게임 기본값 보존을 확인한다. 별도 검사로 setup 부재는 계속 `prepare_failed`여야 함을 확인한다.

- Release 빌드 포함 `SessionStoreTests`: **28통과·1기존 건너뜀·실패0**.
- analyzeManager 단위 검사 전체: **114통과·1기존 건너뜀·실패0**. 기존 건너뜀은 `RejectsLinkedParentDirectory`이며 이번 변경으로 제외한 검사가 아니다.
- 실제 stdio MCP 기본 검사: **16도구·프로토콜2025-03-26·EOF 종료 통과**. `--live` 없는 실행이며 원본/사본 게임은 실행하지 않는다.
- 원시: `extracted/analyze-first-run-1317/`의 `tests.log`·`session-store.trx`·`all.log`·`all.trx`·`mcp.log`.

cpppj 실행 파일과 디컴파일 소스는 변경하지 않았다. 마지막 전체 디컴파일 호스트는 HJOW-Athlon으로 유지한다. DN/WB 제품의 최초 상태 반영은 이 수정과 별개로 각 인계 문서를 따른다.
