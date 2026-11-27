# SID 풀 소진의 현재 World 다리 회수 연결

WB04-E1433, 2026-11-27, HJOW-Athlon. 실제 장기 SurfaceStorage의 SID 풀이 소진될 때 기존 C++ PATCH004af1d0과 같이 현재 World의 다리를 최대50개 가상 삭제로 회수하도록 연결했다. 이전 웹 풀은 예약 꼬리에 도달하면 오류만 내고 회수하지 않았다. 독립 풀의 기존 엄격 검사 경로는 유지한다.

`SidPool.bindAllocation`으로 같은 금지 DWORD·권한·전체 DWORD Bridge TYPE·원문 진단·현재 삭제 경계를 빌린다. 진단의 정상 복귀와 예외, 회수 중 변경된 TYPE/권한/목록·실패 전 부분 효과를 원래 순서대로 보존한다. 반복 대상은 최초 끝번호와 매번 현재 free/TYPE이며, 숫자 SID나 반납 이력을 새 세대로 대체하지 않는다.

Storage는 이미 있는 `RawProcessHostStorage.reclaimForAllocation`으로 현재 World만 선택한다. 실제 World를 반납한 뒤 옛 가상 삭제를 호출하면 거부하고, 같은 긴 물리를 다음 World가 빌리면 새 클래스 호스트로 회수한다. 원래 `releasePatch`·DestroyPatch·일반 객체 수명은 바꾸지 않았다. CD10.72는 웹 범위 밖이다.

원본 관측47행의 반환·전체 풀 Adler·두 삭제 기록·진단/회수 사건 순서가 모두 일치했다. 최대50회·콜백 중 TYPE 변경/새 후보·정상 복귀 진단·예외 부분 상태와 예측 할당을 포함한다. 실제 World의 Content 가상 삭제·전체 DWORD 비교·반납 후 거부·새 대여를 추가 확인했다. 이는 Node의 모듈 통합 검사이며 실제 미션에서 SID를 인위적으로 소진시킨 플레이를 주장하지 않는다.

격리 후보75/75 뒤 본소스 관련/대응표84/84, 선행 ESLint/운영 빌드 종료0, 재번들 바이트 동일성1/1, 기존 C++ 화면/덤프 단언을 유지한 Chromium L2 1/1을 통과했다. 번들은 3,510,896바이트·SHA-256 `0da5a46c2978732ac114f0b83c0a6ba10b392603d2408f6afc4f2f564ef63741`다. v26=3/4·v22=3/8이다.

[Node84](webpj-e1433-evidence/node-focus.log)·[빌드](webpj-e1433-evidence/build.log)·[재번들](webpj-e1433-evidence/bundle-fresh.log)·[Chromium](webpj-e1433-evidence/chromium-l2.log)·[Native47 후보](webpj-e1433-evidence/candidate-native47.log)·[출처/해시](webpj-e1433-evidence/provenance.json). 3.6절의 SID 잔여를 닫고 GameAux의 다른 미확인 소비 경계는 유지한다.

전체 회귀 E1438에서 새47행 자료의 추적 등록 누락을 확인해 목록1331개로 보완했다. 자료/원본 단언은 유지했고 추적+회복5/5를 통과했다. 첫 전체는 부분 실패/중단으로 보존하며 E1438을 다시 실행한다. [보완/원문](webpj-node26-sid-fixture-registration-20261127.md).
