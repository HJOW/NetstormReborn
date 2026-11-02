# Edit 일반 좌클릭과 다리 품질 순환 (단위748)

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 내보내기를 바꾸지 않았다. 이전747의 실제 월드 재그리기 연결·집중 검증·커밋을 유지한다. 현재 실행 제품은 **748 / 표시11.01**이고 전체 목표는 진행 중이다.

## 원문과 수정

`004da228`의 일반 좌클릭은 Ctrl과 현재 special(`0055a480`)로 같은 force 값을 구해 Pick와 `Select(id,force,1)`에 사용한다. 편집 후처리 `004da2e8..004da357`은 **Select 반환값 대신 현재 선택 `005caea0`**을 읽는다. Editor 또는 현재 선택이0이면 종료한다. Ctrl이 없으면 현재 객체의 소유자를 따라 Player Core를 선택하고 같은 월드 재그리기를 증가시킨다.

Ctrl이 있으면 unsigned owner BYTE와 현재 로컬 Player DWORD를 비교한다. 다르면 가상+74로 소유자를 변경하고, **Editor를 다시 조회하지 않고 최초 보관 SID**로 `004d5290`을 실행한다. 가상 효과 뒤 현재 selected가 바뀌어도 같은 보관 SID를 사용한다. 같은 소유자이고 현재 TYPE flags2&4이면 다리 순환 `00422640` 뒤 추적한다. 일반 SetOwner 명령의 뒤늦은 Editor 재조회와 혼동하지 않는다.

기존 `UserInput::Event`의 일반 좌클릭은 `Select(Pick)`만 실행했다. 새 `GameWorld::SelectFromPointer`가 실제 선택 장치와 위 후처리를 같은 Session의 Pool/Player/Core/월드 재그리기에 연결한다. force도 양쪽에 전달한다. 모드1/6의 기존 전용 미복원 경계는 일반 후처리에 섞지 않았다.

다리 순환은 현재 프레임의 플래그20을 먼저 검사하여 Normal로 변경한다. 아니면 현재 TYPE를 다시 읽고 플래그40이면 Cracked를 시도하며, 해당 프레임이 없으면 Normal로 돌아간다. 나머지는 Hard다. 20/40 중첩은20이 우선이다. 플래그 검사 프레임 번호는 frames getter 전에 보관한다. 기존 `RawBridgeEditor`의 수명 WORD 쓰기→NotifySurface→프레임 검색/직접 DWORD 쓰기→가상+88 순서를 재사용하고, Cracked 검색 성공 여부만 반환하도록 했다. 같은 난수를 사용하며 순환 하위에 원문에 없는 owner/Editor/고정 bridgeType 관문을 넣지 않는다.

## 독립 원문 근거

새 `tools/decomp_editor_primary_selection.py`는 게임/창 없이 보호 PE의 x86을 실행한다. 기존 원본과 fixture를 수정하지 않는다.

- 편집 후처리 **256행**: Editor/Ctrl/선택0·50/owner0·1·2·255/TYPE 플래그/가상74 뒤 Editor·선택·owner 변경/월드 감김을 교차했다. 가상74·Core 선택·TYPE flags·다리 순환 호출은 명시 외부 경계이며 후처리의 분기/보관 SID/호출 순서는 실제 원문이다.
- 다리 순환 **120행**:10프레임×TYPE 플래그2×seed3×WORD2다. `422640`, flags 조회, Normal/Cracked/Hard, 코드 후보와 난수는 실제 원문 몸체를 실행했다. 후보 할당1024BYTE/free/가상+88만 기존 명시 경계로 분리했다. Cracked 부재→Normal과 둘 다 부재인 방향도 포함한다.
- 두 새 fixture를 최종 도구로 재관측해 바이트 일치했고 허용 코드가 불변임을 확인했다. 일반 입력 전체·Pick/Select·실제 게임 관측으로 확대하지 않는다.10.72 대응은 이번 실행 범위가 아니다.

## 실제 연결과 회귀 검사

창 없는 실제 **Client→UserInput→GameWorld→Session**과 보호10.78 자산을 사용했다. Gameplay/Kencloud/Subscriber/Core/Player의 실제 공유 소유자를 연결한다. 맵 밖 좌표의 후보0을 disabled 선택이 거부하여 기존 SID를 보존하도록 한 입력에서 plain 좌클릭의 Core 추적과 월드 값 `ffffffff→0`, 별도 미니맵 값 보존을 확인했다. 이 원래 경계는 제품 수정 전 **1검사/2단언 실패**였고 수정 후 통과했다.

같은 입력 부모에서 편집용 새 다리의 Ctrl 소유자 변경과 Normal→Hard→Cracked→Normal을 실제 Session/현재 프레임 표로 확인했다. 사제의 소유권 변경/포획/희생이나 전투/승리를 새로 재현하지 않았다. 이 검사는 **후보0/disabled 선택 보존 후처리**의 실제 접합이며 활성 Select/픽셀 Pick/사용자 GUI 전체의 검증은 아니다.

최종 **집중64개/실패0**이다. 새5개(실제 입력1·후처리/누락 경계2·다리 원문/조회 순서2)와 기존 메뉴/Session·비용Form·수집/부착/카메라·SetOwner256행·전체다리30행·개별다리240행을 포함한다. 기존 집중 실행 대상에 소스만 추가해 실행66개/전체71대상과 주 검사 캐시ON을 유지했다. 전체 C++ 재실행은 아니다.

최초 검사 장치 두 번의 Subscriber/Core-Player 소유자 누락은 `tests-before.log`, `tests-before-v2.log`에 남겼고, 정정한 실제 수정 전 실패는 `tests-before-v3.log`다. 첫 제품 빌드에서 span.at 미지원2진단이 발생해 범위 확인과 operator[]로 정정했다(`build-v1.log`). 최종 `build-v2.log`는 제품/집중 대상 성공·경고/오류0이며 `tests-v2.log`가64개 통과 근거다. 최초 실패를 없던 것으로 처리하지 않는다.

EXE SHA-256: `3471ba2b14c5d6d6417ebcb25f37e407ce3d019eed7dc4f95984c205f48611bc`. 기존 보호 **2656개(AGENTS1·원본1394·CD425·fixture836)** 모두 불변이고 새fixture2를 포함해 **2658개/fixture838**다. 다음 보호 기준은 Git 제외 `extracted/cpp-editor-primary-selection-748/protected-after.json`이다.

이번 새 게임 프로세스/독립CLI/GUI/전체34/ZIP/전체C++/MP 검사는0이다. 마지막 GUI는746제품 화면 조정·재시작이며 전체34/ZIP645는644제품의 별도 근거다. 물리입력/PCM·자연완주·제외 재현·공중3종 전용 행동 검사는 추가하지 않았다.

## 다음 연결

일반 마우스 모드1(대상 취득 `447340`·selectTab.wav·강조0)과 모드6(섬 조회 `4239c0`·명령 `5428a8`·강조0), 우클릭의 현재 모드 관문을 먼저 대조한다. Ctrl+special 영역의 가상+70과 missionPost의 선행 분기도 별도 누락으로 남는다. 이번 후처리 완료를 일반 마우스 전체 완료로 확대하지 않는다. 나머지 UI/Edit/시나리오/파일·수명/호환성과3~8차는 계속 진행한다.

보고서는 `cpppj/editor-primary-selection-20261102.json`, 원시 로그는 Git 제외 `extracted/cpp-editor-primary-selection-748/`다.
