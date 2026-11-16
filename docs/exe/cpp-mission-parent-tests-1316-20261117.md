# cpppj 1316 — 미션 교체 검사의 장기 Template owner 복구

2026-11-17, HJOW-Athlon. 제품1315의 코드는 유지하며, 1314 인계에 남긴 미션 진입 **2검사·5단언 실패**를 해결했다. 마지막 디컴파일 호스트와 일치하고 원문/fixture 변경은 없다.

`MissionAsyncScene`은 Client 수명의 `RawSurfaceStorage`와 게임 전역을 공유하면서, World를 교체할 때마다 암묵적으로 새 `CampaignTemplateStorage`를 만들었다. 첫 World가 표면에 결합한 직렬화 깊이 owner와 두 번째 World의 새 owner가 달라 `BindSerializationGlobals`의 동일 owner 계약에서 중단됐다. 실제 Client는 같은 `CampaignTemplateStorage`를 각 Session에 전달하므로 이 실패는 검사 장치의 수명 불일치다.

`MissionAsyncConnectionTests.inc`에 장기 Template 저장소를 보유하고 `services.templateStorage`로 두 World에 같은 owner를 전달했다. 기존 `RawSurfaceStorage` 계약, 상태7/8의 복사·로그·확정 순서, 안내/접속 실패의 기대값은 바꾸지 않는다.

- Release `netstorm_mission_entry_tests` 빌드 종료0.
- `--filter MissionStartingParent`: **2/2 통과**.
- 해당 실행 파일 전체: **115/115 통과**, 실패 단언0.
- 원시: `extracted/cpp-mission-tests-1316/build.log`·`parent.log`·`all.log`. 수정 전5단언 실패는 `extracted/cpp-first-run-1314/mission-starting-parent.log`에 보존했다.
- 빌드 완료 전에 실행을 요청한 첫 시도는 링크 중 파일 잠금(WinError32)으로 시작하지 못했다. 빌드 종료0을 확인한 후 위 검사를 실행했다.

제품 소스·바이너리 동작·한국어 자산은 이 단위에서 바뀌지 않는다. 새 게임 창/실제 MP 실행은 없으며, 사용자 확인 테스트를 대체하지 않는다. 이 실패 항목은 인계에서 완료로 정리하고, 다음은 analyzeManager 무옵션 준비 지원과 오래된 검사 실패 기준선의 재분류다.
