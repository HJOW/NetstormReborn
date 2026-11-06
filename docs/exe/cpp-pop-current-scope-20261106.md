# 공통 Pop의 현재 연결과 checking 잔여 경계 — 단위957

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 HJOW-Athlon/2026-11-03이다. AGENTS/공통·DN 인계를 다시 읽고3절4번의 앞선 checking/공통 Pop부터 진행했다. 제품 소스와 정밀 산출물은 변경하지 않았다. 제품은956 그대로다.

## 원문 부모와 현재 연결

checking 잔여8개의 현재 분류는 부모004a7510/004b02d0/004dc220, 부분004b54e0/004b8cb0, MP분류0042bc10/004b4df0/004b8b20이다. 895에서 완료 집계한0048fb30/004c0e20을 다시 미구현으로 세지 않는다. 공통 Pop004b02d0은 비멀티 부모와 일부 MP 가상 표를 함께 포함한다. 이번에 대조한 범위는 **현재 비멀티 진입·checking 진단/부분 효과·실제 장치 연결**이고 전체8함수 완료가 아니다.

| 원문 선택 | 현재 구현·실제 소비자 |
| --- | --- |
| SID/void/contained 입구, 좌표 판정/보정 | RunSquidPopParent, 같은 Client.WorldCells getter와 현재 TYPE/로그. 좌표·시작점의 ordered 비교와 저장 범위는889 근거 유지 |
| 좌표/미니맵 → spot/옆면/기존 grid 진단 | 현재 Pool/spot/hash와 RawBuildingSpace. 6b7은 checking과 무관,6e8은 같은 장기 checking이며 진단 복귀 뒤 저장된 후보/현재 TYPE를 구별 |
| SHP/단계/버킷/섬 번호 | 같은 SquidDisplay의 현재 TYPE/프레임 조회 후 실제 hash/WORD 쓰기. 앞선 spot/좌표의 부분 효과를 rollback하지 않음 |
| 가상88/8c → 최초30 → Activate | SquidPop::PopPatch의 실제 선택 위치. 미지원 가상 함수를 공통 함수로 대체하지 않음 |
| Activate/후처리/최종71a | RawSquidActivate → PostPopVirtual → 실제 파생/공통 장부. 같은 World 깊이·현재 서버/TYPE·로딩 비용표·AI/Player 목록·현재 owner/선택 재검사 |
| 장기 대여 | RawSurfaceWorld의 실제 장치와 RawCampaignSession의 같은 Client checking/WorldCells/TYPE·AI/선택 장치를 연결하고 Session 반납 때 참조를 분리 |

PATCH의 실제 Pop은 PopPatch로 반환하므로 아래 CD 본문의 전체 Validate를 실행하지 않는다. PostPopVirtual/PostPopBase의 PATCH에도 명시 Validate를 다시 넣지 않는다. 독립 Activate/Validate API와 CD 한정 사전 계약은 구별한다. 과거 예외 문자열만 검색해 이미 복원한 부모 전체를 다시 미구현으로 판정하지 않았다.

## 현재83표 정적 대조

현재 DerivedConstructors의82 recipe와 base501dd0을 보호 PE에서 다시 읽어83표의 +20/+28/+30/+88/+8c를 보존했다. 생성자/가상 표 이름을 임의 TYPE 추정으로 대신하지 않았다. +20은46종, +28은전부004b0c60, +30은전부004ad470이다. 현재 Unpop 표시 metadata와 두 Mana 우선 분배를 합치면 양쪽 표시 경로의 실효 마스크3은81표, 마스크0은 메타맵2표다. 897에서 복원한 간헐천 +8c004854b0도 현재 mask3이다.

행별 후처리 부류 이름은890의 분류와 현재 소스를 함께 읽어 연결했다. 이 정적 자료는 **현재 모든 파생 몸체를 실행했거나 모든 장치·입력을 검증했다는 증거가 아니다**. 기존 부류 이름과 새 PE/표시 metadata 관측의 출처를 JSON에서 구분했다. 공중3종의3표와 Outpost1표는 계속 코드 대조만 유지하며 전용 검사를 실행하지 않았다.

0050c9f0(TYPE108/109)의 +88=004787c0,0050cc08(TYPE161)의 +88=004791b0은8번 MP/메타맵 잔여다. 두 표의 +8c=004ad670도 지원으로 강제하지 않았다. checking의 다른7함수도 기존8번 이월 분류를 유지한다. 특히 신전 event4/재접속에 직접 inSingle 관문이 없다는 기존 한계는 모든 단독 도달불가 증명으로 바꾸지 않는다.

따라서 checking 집계는 **잔여8=부모3+부분2+MP분류3, 완료부모39/부분4 그대로**다. 4번의 비멀티 checking 연결 점검을 정리하고 전체 잔여8개를8번에서 계속 구현하도록 명시한다. 공통 Pop의 다른 파생·Form/Process·상위 실패 수명은 다음 항목으로 보존한다. 이번 정적 분배 점검으로 전체 Pop/Factory 완료를 선언하지 않는다.

## 실제 집중 재대조와 다음 작업

현재 제품 라이브러리에 netstorm_pop_context_tests를 다시 연결했다. Release 종료0·경고/오류0이며 기존31검사(공통 Pop 부모14/Activate10/세계 크기3/간헐천 부분 표시4)가 실패0이다. 새 검사/Native CPU 실행/fixture는0이다. Native260/432행은 기존 fixture를 C++에서 재생한 결과다. 원문 부모의 현재값 재조회·부분 실패와 실제 Client/World 진단·표시 연결을 포함하지만 81개 가상 표의 실행 결과로 세지 않는다.

912가 남긴 구체적인 다음 범위도 현재 소스에서 확인했다. Process004b1440/CD004af380의 파생 접두/실제 가상 부착과 SquidProcessHost::Attach/Reattach의 선행 검사·복구 정책, ContentFormHost::Attach의 선행 ValidatePop/정렬·체인·부분 쓰기, Factory의 상위 할당/외부 heap/실패 정책이다. 다음958은 같은4번의 공통 Pop/Factory 공유 수명 잔여에서 이 부모들을 원문과 대조한다. 기존 Form 공통 부착/Pop의911/912 복원을 같은 상위 부모 전체 완료로 확대하지 않는다.

게임/GUI·포획/희생·특정 적 공격/승리·공중3종 전용·신규 MP/패킷/통신·물리 입력/PCM/CD 삽입·전체 CTest/34미션/ZIP은 실행하지 않았다. 원시와 다음 보호 기준은 `extracted/cpp-resume-957/`이다. 제품956/11.01의 EXE `23d356438e3d85b998965b9f49831574530f554e95447ed1510e8c19616f7b77`,11,281,408바이트와 보호2789개/fixture969개가 불변이다. [완료 JSON](../../cpppj/pop-current-scope-20261106.json)을 따른다. 전체 목표와 공통 Pop/Factory 잔여는 계속 진행 중이다.
