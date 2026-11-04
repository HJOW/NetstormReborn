# 단위877 — 의식 저장소·공통 WORD와 Player 결과 주소

다이스·제단·내용물 부모의 WORD 지정을 같은 primitive와 현재 Bridge 전역에 연결했다. PATCH plain 쓰기와 진단 복귀, 가상 Count 앞의 Player 결과 주소도 원문 순서로 대조했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**, 현재 호스트도 동일하며 원본/정밀 산출물은 변경하지 않았다.

## 원문과 구현

`SetRawSquidWord`는 `004ac220`/CD `004abb00`처럼 WORD 두 바이트를 먼저 쓰고 현재 TYPE BYTE와 전체 Bridge DWORD가 같을 때만 표면을 통지한다. 선택된 장치가 없어도 앞선 WORD를 되돌리지 않는다. PATCH는 현재 물리 슬롯을 쓰며 CD는 기존 할당 계약을 유지한다. PE `005411a0`/CD `0051ca8c`의 초기82도 확인했다.

다이스 희생 완료의 제단0, 해방 사제의 원래 WORD 복사, 내용물 부모 알림의 원래 owner WORD를 이 helper로 연결했다. 실제 세 상태/장치는 같은 `core.bridgeState.bridgeType` 주소와 `core.building.NotifySurface`를 사용한다. 따라서 콜백이 표시 TYPE를 바꿨을 때 Native 공통 setter의 다리 후속도 선택한다. 모든61caller를 이번에 새로 연결한 것은 아니며 다른 모듈의 기존 setter는 그대로다.

다이스 extra2/해제와 제단·내용물 plain frame 쓰기는 PATCH 물리 슬롯을 사용한다. 희생 완료의 altarSid0은 PATCH에서 `Dais.cpp:0x2ba` 진단이 정상 복귀하면 원래 저장한 SID로 다음 효과를 계속한다. CD 기존 예외 계약은 유지한다. 해당 포획/해방/희생/의식 효과와 진단 본체는 실행하지 않았으며 코드/정적/컴파일 근거다.

`00449b30`은 두 번째 Count 앞에서 Player 결과 DWORD 주소 전체를 저장한다. CD `00482620`은 owner BYTE만 저장하고 Count 뒤 현재 Player 저장소에 쓴다. `PlayerResultSlot()`은 정확한 네 바이트 보기이며 PATCH는 이를 앞에서 보관한다. CD는 기존 현재 setter로 돌아간다. 독립 DWORD도 unsigned BYTE 객체 표현으로 지정 폭만 쓴다. 마지막 정밀 C에 없는 내용물 알림 `00449660`은 PE의 bounded 기계어로 보존했다.

첫 빌드 후 정적 검토에서 Dais/Altar의 현재 TYPE가 프레임 표를 벗어날 때 배열 밖을 읽을 수 있는 getter3곳을 확인했다. 선택한 조회 위치에 명시적 물리 범위 오류를 추가했고, 표를 확대하거나 TYPE를 다른 번호로 바꾸지 않았다. 정상 타입의 원문 순서는 유지한다.

## 검증과 한계

Release2회는 **143.717초 / 36.534초·모두 종료0·경고/오류0**이다. 첫 빌드 뒤 위 정적 범위 검토로 제품2소스/새 검사1곳을 갱신했다. 검사는 최종 빌드 뒤 처음 실행했으며 **고유27개/10묶음(새3), 첫 실패0**이다. 고정 최종9소스와 현재 해시가 같고 이후 교정0·기존 기대값 변경0이다.

새3개는 WORD 저장 전후의 통지/누락 장치·인접 바이트, Player 기록 교체 뒤 이전 주소/현재 setter·정확한 DWORD 폭, 범위 밖 현재 TYPE에서 효과 전 실패만 검사한다. 실제 포획/해방/희생·의식 본체는 호출하지 않았다. 기존 읽기/VT/번호/현재 상태와 수명1,928행의 독립 배열/실제 Player 기록 재생도 통과했다. 수명 접두는 마지막 Damageable 진입에서 중단하며 실제 전체 파괴 검증이 아니다.

새 실제 Session/Native/fixture0·보호2,761개/fixture941개 전부 불변이다. 제품877/11.01, EXE **10,613,248바이트**, SHA-256 `0774e466b8c21d5ac28096c24a65d5cb9f584b2f89a00cd5cb0543200cb65e6c`다. checking10(부모4/부분3/MP3)·완료부모38/부분3 유지다. 전체 Pop/전체 동일성 완료가 아니다.

[JSON](../../cpppj/ritual-storage-current-20261105.json)과 Git 제외 `extracted/cpp-resume-877/`에 첫/최종 소스·빌드·첫 검사·원문/보호 해시를 보존한다. 실제 게임/GUI·특정 적 공격/승리·신규MP/패킷/통신·공중3종 전용·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은0이다. 다음878은 내용물 부모 알림의 PATCH 선택 장치/효과 전 반환을 대조하며 실제 포획 알림 효과는 코드/정적으로만 다룬다. 다른 TYPE/모드·TYPE 초기화 writer·Teleport 다른 생성 함수·전체Pop/Factory/군도·메뉴/Edit/State/파일·Windows·후속 단계는 남는다.
