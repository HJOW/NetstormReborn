# Player 정기의 현재 장기 소유자 연결 — 제품1245

제품1244 뒤 Player `0050ee08 / 00490810`의 공용 분기와 실제 현재 장치·소유권을 연결했다. 전체 목표와 M6/NoWorld는 진행 중이다. [확정 보고서](../../cpppj/player-regular-owner-20261110.json)의 원시 경로는 `extracted/cpp-resume-1245/`다.

## 원본과 현재 연결

| 원본 분기 | 현재 구현 |
| --- | --- |
| local owner가 다르거나 count0 | 같은 Client `PlayerWorldState` 두 owner 주소를 alias로 보유한다. 사본/Player9/임시 World 없이 원래 payload를 반환한다. |
| 사건1 | 로그29의 원문 세 문자열과 현재 battle 재조회를 보존한다. 시작 시 실제 확보한 메모리에서 사건2/payload60을 한 번 구성해 같은 Host/현재 Regular 역할·시각으로 부착한다. 최초 Player PostPop 예약도 같은 확보 경로를 사용한다. |
| 사건2 | 현재 battle일 때만 실제 Runtime을 확보한다. 기존 Template Get→반환 State첫BYTE에4 설정→다시 읽은 현재 Template owner의 Changed 전체 동안 owner를 유지한다. 실제 getter/Notify 함수도 선택 시 사본으로 보유한다. |
| 기타 사건 | 원본 `0 / \Ns\O\Player.cpp / 41d` 진단 후 payload 반환을 유지한다. 같은 Client 진단과 실제 로그 필터/FILE 수명을 사용한다. |

`RunPlayerRegular`가 Native 장기 입구와 기존 typed wrapper의 같은 분기 본문이다. typed/CD의 기존 Player VT/할당 검사를 유지했다. Native는 전송 장치 설치 뒤/첫 Host·메뉴 전에 현재 전역만 결합하며 값을 초기화하지 않는다. Native Session은 별도 지역 정기를 만들지 않는다. Template의 큰 실제 본문만 weak 대여·전체 Runtime pin으로 연결하며 강한 Session 역소유나 빈 성공을 추가하지 않았다.

실제10.78과 CD10.72의 가상 표·함수·전체 C/PE 범위를 읽기 전용으로 확인했다. CD 대응은 `005002a8 / 00407ef0`이며 같은 두 사건/60초 payload/State OR4와 별도 owner 재조회다. **이번 Player 정기 오라클·MP 단위/패킷/통신/게임/기록 실행은0**이다. 기준 커밋 c54f488c의 기존 본문과도 지역명·추가 Template pin을 제외한 분기 토큰이 같음을 대조했다. 정적 대조를 실행 검증으로 표시하지 않는다.

## 빌드와 허용 검증

- 최초 빌드는 117.694초/종료1이었다. CD의 nonconst `AllocatedBytes`를 const Pool에서 호출한 오류와 당시 소스·로그·고정SHA를 `first-build/`에 보존했다. 기존 판본별 Handles를 유지한 채 공용 분기만 추출해 교정했다. Native 장치 선행 순서도 정적 재검토에서 교정했으며 잘못된 순서로 GUI를 실행하지 않았다.
- 최종 Release/x64 제품2·집중1대상 **759.944초/종료0·경고/오류0**, 빌드 고정13파일 불변이다. 이 가운데 RawPlayerWorld의 두 원문은 기존 계약 그대로 보존했다.
- 관련 **기존 비MP256/256**을 통과했다. Player 정기 검사3개는 변경 인터페이스의 **컴파일만** 확인했고 실행 목록에서 명시 제외했다. 이256개를 MP/Player 두 사건의 동작 검증으로 세지 않는다.
- 최종 EXE로 Tutorial4 정상 UI의 Wind Generator 생산·건설 요청→TYPE75/owner1 생성→600프레임 대기→종료0을 확인하고 PNG를 열어 확인했다. 건물 완공·전체 원본 동일성/음향 검증으로 확대하지 않는다. 포획/희생·특정 공격·승리·자연 완주·공중3종 전용 실행은 하지 않았다.
- 기존 원본/CD/fixture·AGENTS **2,808파일 불변**. 사용자 프로필과 시험 데이터는 분리했다. 새 배포ZIP은 만들지 않았다.

## 남은 범위

알려진87VT의 정기 분기 연결은 **80/87**, 잔여 **7VT/5함수**다. 다음은 공격형 ArcSpire/AcidFence와 WindFlyer, 메타4VT의 현재 장기 연결이다. Template Get/Changed의 큰 State 생성·확장·저장, 사제/일반601/의식 하위·최초 후처리·큰Path/AI/배치/FortFinish·전체 processVT/호출자/owner 감사·M6-l/m은 별도로 남는다.80/87을 전체 NoWorld/MP 완료로 세지 않는다.

현재 호스트는 마지막 Client 양판본 디컴파일 `2026-11-03 / HJOW-Athlon`과 같다. R전용 마지막 디컴파일은 `2026-11-07 / 같은 호스트`다. 새 디컴파일·원문 변경은0이다.
