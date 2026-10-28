# 모델 용량 오류 뒤 작업 파일 소실 점검

2026-10-28, HJOW-Athlon. 사용자가 반복되는 `Selected model is at capacity` 메시지 뒤 파일 소실 점검을 우선 요청했다. 진행 중543의 UI 수정·실행은 멈추고 파일/커밋/해시를 대조했다. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 산출물은 변경하지 않았다.

## 확인 결과

| 대상 | 결과 |
| --- | --- |
| 현재 HEAD 추적 파일18,593개 | 사라진 파일0, 원래 내용이 있던 파일의0바이트 손상0 |
| 완료542 소스·검사·fixture2,930개 | 누락0. 현재 작업 중인 아래3파일만 변경됐으며 예상 밖 변경0 |
| 보호 원본/CD1,819개·AGENTS.md | 누락/해시 변경0 |
| 완료541/542의 원시 근거62개 | 누락/해시 변경0 |
| 완료 커밋 | `1ddc4ba0`, `55520f78` 모두 현재 HEAD의 조상으로 보존 |
| Git 객체 연결 | `git fsck --connectivity-only --no-dangling` 종료0 |
| 현재 작업4파일 | 별도 사본·SHA 보존, 현재 파일과 사본 모두 일치 |

이전에 실제 관측한 소실은 **파일 삭제가 아닌 미커밋 변경의 원복**이었다. `GameMenuCommands.cpp`의 명령 이름 연결과 `ProductionInitialContextTests.cpp`의 신규 검사가 같은 시각에 HEAD 내용으로 돌아갔다. 사용자는 직접 복원하지 않았으며 다른 세션은 dotnetpj 작업 중이라고 확인했다. 모델 용량 오류가 직접 원인인지는 확정하지 않았다.

해당 변경은 복구돼 있으며 현재 다음4파일을 보존했다.

- `cpppj/src/client/GameMenuCommands.cpp`: 원본 `ToggleBottomGump` 이름 연결.
- `cpppj/src/client/UberGump.cpp`: 원본 효과 뒤 반환0을 유지하는 하단 전환의 호스트 분배.
- `cpppj/tests/ProductionInitialContextTests.cpp`: 시나리오 명령·드래그·좌표 보존 회귀.
- `tools/cpp_chat_divider_smoke.py`: 사본 시나리오 버튼을 이용하는 실제 입력 검사 도구.

## 복구 자료와 현재 빌드

[복구 패치](recovery/cpp-chat-divider-543-20261028.patch)는 위4파일만 포함한다. 현재 변경이 적용된 상태에서 `git apply --check --reverse`가 종료0인 것을 확인했다. **현재 트리에 다시 적용하지 않는다.** 이 패치는 검증 중인543 작업의 보관본이며 검사 통과 제품의 패치로 분류하지 않는다.

작업 PC의 `extracted/cpp-work-loss-audit-20261028/current-work/`에도 같은4파일의 전체 사본을 보존했다. 완료 제품542의 소스는 `55520f78`에 남아 있다. 현재 Release 실행파일은543 검증 중 빌드이며542 완료 해시와 다르다. 이는 진행 중 빌드가 덮어쓴 결과로, 실행파일 삭제와 구별했다.

543의 신규 회귀는 현재 좌표 관련3체크가 실패한 상태다. 실제4:3 UI 실행은 종료0이지만 드래그 좌표 판정이 실패했으므로 수락 완료로 세지 않는다. 다음 작업은 원본 anchor 좌표와 검사 스크립트의 누름/폴링 입력을 대조하는 것이다. 일반 캠페인에서 채팅 메뉴를 숨기는 원본 조건은 유지한다.

dotnetpj의 진행 중 변경·새 파일은 존재만 확인했고 수정·복원·stage하지 않았다. 점검 중 원본/클론 게임 실행·빌드·C++ 검사·디컴파일을 추가 실행하지 않았다. [점검 도구](../../tools/cpp_work_loss_audit.py)·[보고서](../../cpppj/work-loss-audit-20261028.json)를 참고한다.
