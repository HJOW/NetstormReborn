# Edit 품질 명령·Guide 수명·메뉴 종료 통합 — 466

작업 PC는 HJOW-Athlon(192.168.0.94)이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 원본·디컴파일 파일은 변경하지 않았다.

최종 실행 파일 SHA256은 `c8739bd5d0ada3b4a8d675042203cdfe647f768fcecb70dc85de1a42a562fcd9`다. Release build1·2 모두 종료0·경고/오류0이며 각 빌드 중 동결 소스는 변경되지 않았다. 결과는 [통합 보고서](../../cpppj/editor-runtime-integration-20261026.json), 원시는 `extracted/cpp-editor-runtime-466-*`에 있다.

## 실제 구현

- `RawBridgeEditor`와 실제 Editor 메뉴의 Cracked/Normal/Hard를 같은 SID 풀·TYPE·공통 게임 난수·가상 표시 갱신에 연결했다. 수명/직접 프레임 쓰기와 원문 iterator를 유지한다. [다리 품질 문서](cpp-editor-bridge-quality-20261026.md)를 본다.
- Add Island의 필수 하위인 양판본 배치 가능 검사 `CanPlaceIsland`를 복원했다. 실제 Add Island 창은 아직 미연결이다. [섬 배치 문서](cpp-editor-island-placement-20261026.md)를 본다.
- Guide 스크립트의 등록 해제→Native mission0→Config0을 실제 해제 시점에 연결했다. 이전 World를 반납할 때 Tutorial/Start/사용자 입력의 참조만 먼저 닫고 gate/state/스크립트는 원문 이름 비교 뒤 closeGuide까지 유지한다. 기존 튜토리얼 허용표·번호·단계 소멸 효과도 그 시점으로 옮겼다. [Guide 수명 문서](cpp-guide-mission-release-20261026.md)를 본다.
- World 이탈→메인 메뉴는 기존 전체 Native cleanup을 재사용하고 실제 Client의 모드 조회를 사용한다. 전체 Native State3의 복원을 완료했다는 뜻은 아니다.
- 메인 Quit는 실제 RequestQuitApp→모드 판정→QuitApp(99)→종료음→즉시 반환을 사용한다. 종료 뒤 같은 배치의 후속 사건을 소비하지 않는다.
- Script가 없는 Editor도 HTML 취소/Blank 뒤 같은 World의 `editor` 표시로 돌아간다. 현재 World를 메인 메뉴로 오인하던 표시 분기를 수정했다.

## 독립 관찰과 C++ 검사

다리 품질은 실제 PE30행/30정상 반환, 섬 배치 판정은 양판본4,896고유 입력/두 x87설정9,792정상 반환을 독립 관찰했다. 기존 Native fixture는 변경하지 않고 새 fixture2개만 추가했다.

최종 제품의 집중 검사는 **103고유/112관측·실패0**, 새 사례9개 전부 통과다. 같은 제품의 전체 CTest는 한 wrapper 안에서 **3,165개·실패0**, 344.77초다. 이전 실제 등록3,156개를 모두 유지했고 전체 성공 이름 집합도 현재 실제 등록과 대조했다. 수정한 검사3파일의 기존 도우미/검사 본문은 그대로 보존했다.

검사기에서 실제 등록과 다른 필터 `MissionTransition`/`MissionInit`를 지정해 각각0개 실행 오류가 있었다. 최초 원시는 그대로 두고 실제 `GuideTransitions`/`TutorialInit`만 보완 실행했다. 이미 통과한 집중 검사와 전체 CTest를 반복하지 않았다. 전체 로그의 이름3개는 stdout/stderr 진단이 이름 사이에 섞여 있었다. 원시에 관측된 ASCII 접두/접미와 완전한 이름을 해당 성공 marker 구간 안에서 분리해 대조했으며 정규화 내역을 JSON에 남겼다. 제품·기대값·검사 본문은 바꾸지 않았다.

## 같은 제품의 실제 GUI

검사에는 데이터 복사본과 끝 `\`가 있는 게임 경로를 사용했다. 실제 원본 실행 파일과 명시 test-state 호출은 사용하지 않았다.

| 경로 | 관측 |
| --- | --- |
| 메인 Quit 두 실행 | 종료0, 다음200프레임 사건 미실행, 같은 복사본 재실행 뒤 다시 종료0 |
| 실제 Edit→Battle1→Set All Bridge | 세 실행 모두 State4·실제 현재 Fort·HUD·양끝을 유지. 다리314개 대상에서 Cracked314/Normal222/Hard92개 프레임 변경 |
| 실제 TYPE 프레임 대조 | 세 실행의 모든 다리를 보호 원문 TYPE의 방향/번호/플래그/후보 표와 대조. SID·소유자·좌표 보존 |
| Editor Quit Game→No | QuitEditMode 표시 뒤 같은 Editor World/State4/표시 복귀 |
| Credits | Patch4쪽·Original14쪽의 실제 More/Back/Cancel/최종OK, 메인 복귀와 Quit |
| Early1→메인 메뉴→Early1 | 실제 Leave Mission/확인 버튼, 연결표 반납과 새 연결 토큰, 재진입·정지 해제 |

총 GUI8실행을 확인했다. Hard 화면, Credits, 취소 뒤 Editor와 복귀한 메인 메뉴 BMP를 직접 검토했다. 원본 픽셀 동일성의 완료 근거는 아니다.

추가로 원본 타이머1·단계/SP/완료값 주입 없는 기존 입력 계획에서 자연 **Early2 A~I 완주→Tutorial3 진입**을 통과했다. SP10000→3075, 발사기4→3, 회수 환불75, 다음 미션 SP2000을 확인했다. 기존 완료값1은 보존했으므로 영구 진행0→1 저장의 증거는 아니다.

모든 GUI/자연 진행 뒤 원본과 데이터 복사본의 전체 보호 바이트가 같았다. 빌드 전부터의 기존 보호2,474파일과 최종 소스/검사자료2,725파일도 대조했다.

## 다음 경계

전체 목표는 계속 진행 중이다. SaveGoMain의 Yes/No는 실제 전체 저장 뒤 next3 예약과 Native State3 prefix/설치/공통 확정/다음 Poll4까지 연결해야 한다. [다음 복귀 작업](cpp-editor-save-go-main-next-20261026.md), [Save as](cpp-editor-save-as-next-20261026.md), [로컬 AI 검사 옵션](cpp-test-local-ai-integration-next-20261026.md)에 정확한 후속 근거를 기록했다.

새 맵·Add Island GUI·Save as·Test Battle·시나리오, 기존 저장 상태 복구, Credits20초 자동롤, Tutorial 소멸의 denySalvage/Ascend 장기 소유자·늦은 AI 부모, 다른 State1/7과 전체 미션 자연 완주·화면/음향 동일성이 남는다. 최초 MainMenu의 여섯 전역 전체와 Native State3 전체도 완료로 합산하지 않는다.
