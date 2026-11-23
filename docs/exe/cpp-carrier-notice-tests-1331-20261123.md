# cpppj1331 — 포획 내용물 검사의 안내 장치 연결

2026-11-23, HJOW-Athlon. 제품1330과 디컴파일은 변경하지 않았다.

1328에서 실행한 `netstorm_menu_session_tests`256건 중254건이 통과했고 `CampaignCarrierFalling_ActualCapturedPriestRelease`·`CampaignCarrierFallStart_ActualSelectedMovementReleaseAndFall`은 `포획 안내 실제 Player 이름 미연결` 예외로 끝났다. 제품 `CampaignClientServices.cpp`는 실제 Client의 Player/Chat 이름 조회와 Tell을 제공하지만 이 두 독립 검사에는 해당 서비스가 없었다.

검사에만 현재 실제 Session의 Player 이름32바이트/NUL을 읽는 장치와 Tell 요청 수집기를 제공했다. 게임의 생성·이동·낙하·안내 판정은 그대로 실행한다. 기존 모든 내용물/소유권/낙하/프로세스/선택 해제 검증을 유지하고 실제 안내 문자열도 추가로 단언한다. 제품에서 누락을 숨기는 fallback은 넣지 않았다.

처음 추가한 기대값은 Create의 두 번째 인자2를 소유자2로 잘못 해석해 간수 이름/색을 기대했다. 실제 인자는 공장flags이며 생성 직후 캐리어 소유자는 중립0이다. 실제 요청 `WarnYourPriestTaken(1,,You,~q,~l)`과 현재 필드를 확인하고 중립 이름·색/이전 사제1을 기대하도록 교정했다. 두 번째 검사의 SetOwner(1)은 내용물 생성 **뒤**에 있어 선행 안내는 역시 중립 포획자0이다. 최초 예외·추가 단언 실패·실제 문자열·최종 통과 로그를 모두 보존했다.

Release 집중 대상 빌드 종료0, `--filter CampaignCarrierFall` 관련3건 모두 통과했다(실패했던2건+기존 선택 만료1건).256건 전체를 반복하지 않았고 전수 통과로 확대하지 않는다. 창/게임/통신을 실행하지 않은 독립 통합 검사이며 사용자 자연 플레이 확인을 대신하지 않는다.

[검증 JSON](../../cpppj/carrier-notice-tests-1331-20261123.json). 원시 자료는 `extracted/cpp-followups-1328/netstorm_menu_session_tests.log`, `carrier-fall-first-assertion.log`, `carrier-fall-observed.log`, `carrier-fall-final.log`다. 마지막 디컴파일 PC는 HJOW-Athlon으로 유지한다.
