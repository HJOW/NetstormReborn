# 비멀티 미션 State 범위 정리와 State2 연결 — 단위956

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 HJOW-Athlon/2026-11-03이다. 정밀 산출물은 수정하지 않았다. LEFT_JOBS 3절3번을 앞부터 진행한944~955 뒤에 남은 Native3/State1/단독State7/cleanup Tick의 현재 연결을 대조했다.

## 찾은 State2 제한과 수정

보호 PE `004b6e46..004b6e5b`는 current에서1을 빼서42칸 점프 표 `004b874c`를 선택한다. State1/2의 두 항목은 모두 `004b6fa9`이며, 정밀 C도 case1/2/3/6을 같은 큰 본문으로 분배하고 State1/2에 같은 Editor 모드를 설정한다. 이 자료는 정적 분배 근거이며 새 CPU 실행/자연 UI 도달 관측이 아니다. 원시 표와 명령은 `extracted/cpp-resume-956/native-switch.json`에 보존했다.

현재 Session의 파일/월드/후반 UI/전체144BYTE 확정은 이미1/2를 지원했지만, 호스트 Poll의 선택, `InstallExistingEditorState`, `PrepareNativeEditorEntry`, 파일 전 HUD scope는1로 제한돼 있었다. 직접 State2가 요청되면 실행되지 않거나 current1 계약에서 거부됐다. 이 네 위치를 원문 공통1/2로 연결했다. State3/6을 Editor 앞단으로 넣지 않고, 실제 준비 대여·동일 Client owner·한 번 HUD·미완료 입력 차단은 유지한다. State2도 미션 조기 ret이면 다음 Tick의7에 넘기고, 일반 설치면 실제 공통 commit 뒤 GameWorld/입력을 공개한다.

새 검사는 실제 Client/준비 Session의 State2 → 공통 출처/배경 앞단 → 실제 Deck/HUD scope → 파일/checkpoint/월드 → 후반 UI → RawLocalMissionState 전체 확정 → Commit → 다음 Poll4를 연결한다. 수정 전 제품 라이브러리에서는 이 검사1개가 실제 current1 계약에서 실패했다. 그 원시는 `state2-control.log`이며, 수정 후 같은 검사가 통과했다. 기존 State2 거부 검사는 Editor 외 State3 거부로 바꾸어 제한을 유지했다.

State2의 자연 UI writer는 관측하지 않았다. GUI의 현재 State를 강제로 바꾸거나 새 메뉴를 만들지 않았다. 이번 수정은 원문에 있는 분배의 복원이며 자연 도달을 입증한 결과로 세지 않는다.

## 3절3번 현재 범위 판정

| 비멀티 선택 | 현재 실제 연결·근거 | 남는 범위 |
| --- | --- | --- |
| Native3 intro0 | 실제 즉시39와39→40→41→3 부모/영상 수명 | 다른 영상/장치 실패는 기존 진단·호환성 범위 |
| Native3 Welcome/Blank/Startup/Tips | 같은 Config/Guide/HTML/tail/다음4, 최신 C++ Native11검사와 기존 PE82행 | 서비스 종료 검증 안내는 사용자 지시로 비활성화. 존재 saved-state의 일반 전투 복구는8번 |
| Native3 결과1/2/source | 원문 삭제/배경/음악 순서와 현재값 재조회, 실제 Client 장치 연결 | 실제 승리·패배/모든 배경 픽셀은 사용자 확인이며 재현하지 않음 |
| Native3 제목 뒤 다른 Fort | 951 반환계약 →952 같은 실제 파일/Core 본문 →953 기술/사제·정상확정 | 특이 분기의 자연 도달 미입증. 내부 하위/진단 전체는4번 |
| State1/2 공통 Fort | 준비/이름/기존·신규·손상 파일 반복/미션 조기 ret/후반 UI/commit,956의 직접2 진입 포함 | 모든 맵 조합·자연 플레이 동일성은5·6번 |
| 단독 State7 | 장기 Client 부모가 복사/로그/cleanup1/World 설치 후7→8을 확정하고 현재 World로 접속, 기존8/9/10/13/14 분배기와 다음 Tick | 일반 전투/재접속/Zone0/시계 패킷·메타 Poll은8번 |
| Tick/연속 수명 | 954의 새 예약 지연/부분 Fort 공개 차단,944의 실제3세대/이번 재대조,945/949/954의 실제 왕복 이력 | 장시간 모든 전환/실제 PCM·물리 입력·전체 동일성으로 확대하지 않음 |
| LeaveBattle | 949 사용자 경로,950 Guide 조회,955 양수22행/두삭제·CD 차이 | 원격 페널티/State6은8번. CD 전체 Native 이관은 차후10.72 복원 |

`CleanupClientStateWorld`의34개 Effect case를 현재 enum과 전수 대조했다. 실제 Client의 전송/정지·커서/시계·Player/AI·Deck/Kernel·HUD·Core0/Runtime 반납·Pool/목록/그래프·새 Player/옵션/기한 장치에 연결돼 있다. saveFort도 실제 현재 Session snapshot을 쓴다. 별도 장치로 남은 saveBattle은 비mission Battle phase14, leaveMetamap은 metamap 선택이므로8번이다. `MapMode`의 반환은 보호 PE `004437f0`의 실제 C3이며 누락 하위의 빈 성공이 아니다. 정적 case34개 대조를34개 새 실행 검사로 세지 않는다. 과거428의 미연결 문장보다 현재 소스/후속 연결 기록을 우선한다.

Runtime 조정기는 같은 Session/표면 세대/Player/Kernel을 확인하고 여덟 AI/Player links/단일 Kernel의 실제 정상 반환 및 known0/Core0 뒤에만 소유자를 반납한다. 소유자 반납 뒤 이전 Session을 읽지 않고, 그 뒤 Pool/Player를 재구성한다. 실제3세대 검사와 조정기/개별 반환 검사를 현재 제품 라이브러리로 다시 링크해 대조했다. 큰 내부 공급자/진단·Factory/파일/팔레트·Chat 잔여는 원래4번이며 이번 State 외곽 완료로 합산하지 않는다.

이 범위로 **3절3번의 비멀티 구현·대조를 정리**한다. 일반 전투/State6을 미션 검사로 완료 처리하지 않고, 자연 완주/모든 상태 조합·픽셀 동일성은 기존5·6번 사용자 확인을 유지한다. 4번의 실제 내부 잔여는 다음 순서로 진행한다.

## 최종 검사·보존

제품과 집중 대상3개, 총4개 Release 대상이 경고/오류0으로 빌드됐다. 고유38검사(새1/기존37) 실패0이다. prefix4/UI확정1/continuation4/실제GameWorld대여1/NativeMainMenu11/StatePoll4/MissionLifecycle2/RuntimeCoordinator4/GuideWorldLifetime4/개별RuntimeRetirement3이다. GuideWorldLifetime은 Begin/Restart/Fork/Close/Init/cleanup/재적재의 세 연속 세대도 포함한다. 기존 PE82/831행은 C++ 재대조이며 새 CPU 관측/fixture는0이다.

실제 게임/GUI·포획/희생·특정 적 공격/승리·공중3종 전용·신규 MP/통신·물리 입력/PCM/CD 삽입·전체 CTest/34미션/ZIP을 실행하지 않았다. 마지막 GUI는954 당시 제품의 실행 이력이고956 실행 결과가 아니다. 최종 빌드/필터별 검사/수정 전 실패/정적 감사 원시는 `extracted/cpp-resume-956/`이다.

제품956/표시11.01, EXE SHA-256 `23d356438e3d85b998965b9f49831574530f554e95447ed1510e8c19616f7b77`, 11,281,408바이트다. 보호2789개/fixture969개 불변·새fixture0이다. 다음 보호 기준은 `extracted/cpp-resume-956/protected-final.json`이다. [완료 JSON](../../cpppj/mission-state-scope-20261106.json)을 따른다.

다음957은 **LEFT_JOBS 3절4번의 첫 미완료 checking/내부 수명 항목**이다. 1~3번의 정리와4번의 전체 내부 복원 완료를 구별하고4→7→8 순서를 지킨다.
