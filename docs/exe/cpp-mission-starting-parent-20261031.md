# 단위660 — World 교체를 감싸는 같은 State7 부모

2026-10-31, HJOW-Athlon(192.168.0.94). 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 원본/디컴파일 파일은 변경하지 않았다. 해시와 원시 로그는 [mission-starting-parent-20261031.json](../../cpppj/mission-starting-parent-20261031.json)에 있다.

## 최초 복사·로그와 정리를 한 부모에 연결

659까지의 실제 호스트는 cleanup 앞단과 새 로그인 안의 공통 Run을 나눠 실행했다. 이번에는 `RunCampaignMissionStarting`이 Client/장기 Surface/다섯 State를 빌리는 부모를 만들고 `PollStarting` 안에서 전체 미션 몸체를 실행한다. 같은 PollBody가 next를 current로 소비하고, 같은 RunBody가 next/immediate의144BYTE 복사·0 쓰기·State7 로그를 **cleanup보다 먼저** 수행한다. 부모가 빌리는 데이터는 기존 World/Session보다 오래 살며 같은 Surface 소유권도 호출 종료까지 보유한다.

`CaptureCampaignMissionStarting`은 이미 소비한 current7의 기본 분모/지역 값만 캡처한다. 이전처럼 next를 다시 소비하지 않는다. 몸체는 기존 World를 반납하고 새 World를 만들며, 새 `PrepareLocalConnectionForState`가 Zone/Renderer/UI/안내/팔레트 후반까지만 수행한다. 새 Session 안에서 Run7/8을 중첩하지 않는다. 같은 외부 부모가7을 확정하고 즉시8에서 현재 새 World의 로컬 접속을 호출한 뒤8을 확정한다. next9는 다음 정상 Tick까지 남긴다.

정리 중 current 인자가 바뀌어도 앞서 복사한 immediate는 이전 값을 보존한다. 수정된 current는 phase7에, 최초 immediate의 인자는 phase8에 남는다. 이 차이는 앞단 뒤에 다시 Run을 시작하면 없어지므로 새 검사가 전체144BYTE로 구별한다. phase/old를 미리 성공 상태로 쓰거나 반납 후의 Session을 부모로 계속 사용하지 않는다.

독립 편의 `RunStarting`/`StartLocalConnection`은 기존 경계를 유지한다. 실제10.78은 새 외부 부모/후반 준비 API를 사용하고10.72의 기존 동기 어댑터는 유지한다. 외부 TCP/IP 주소/포트 해석을 복원한 연결로 세지 않는다.

## 예외와 진행 안내

후반 실패는7 확정 전에, 후반 복귀 뒤8 접속 실패는7 확정 뒤/8 확정 전에 멈춘다. 앞선 원문 쓰기는 보존한다. 후반 RAII에서 정상 안내 수명을 인계한 뒤 접속이 실패할 수도 있으므로, 외부 부모의 catch가 남은 실제 안내를 닫는다. 정상 상태13/타이머 닫기와 이탈 정리는 계속 사용한다. 이 catch의 실제 HWND/안내 픽셀 실행을 새로 검증했다는 의미는 아니다.

## 원문과 집중 검사

보호 PE의88개 명령을 실행 없이 확인했다. Poll의 `004b892a`는36DWORD next→current, `004b8938`은 Run으로 분배한다. Run의 `004b6e15`/`004b6e26`은 current→next/immediate 복사이고 `004b6e41` 로그가 그 뒤에 있다. 공통부는 body 뒤 phase→old/current→phase를 복사하고, 즉시 요청이 있으면 immediate→current를 복사해 다시 같은 Run으로 간다. 새 PE 실행 관측/fixture는 없다.

집중52등록 중 **최종 고유10개/실패0**이다. 새2개/입력3개·기존8개이며 다른42등록은 실행하지 않았다.6회 실행/10함수 호출이며 최초 실패는 없다.

- 새 성공 입력은 같은 State/Gameplay/Surface를 빌리는 이전 **deferred World**를 파괴하고 새 World를 구성한다. 최초 복사/로그 당시 이전 World가 살아 있고 앞단 캡처도 없음을 확인한다. 몸체에서 current만 바꾼 뒤, 안내 시점의 미확정 상태와 모든144BYTE의 서로 다른7/8 인자·다음9를 비교하고 실제 로컬 패킷→phase14/완료 공개까지 진행했다.
- 새 실패 입력2개는 안내 후반 실패/8 접속 실패다. 각각 이전 phase/old 유지와 phase7 확정의 차이, current7/8·immediate8/0, 미생성 커서/패킷0·부분 World 대여 반납을 확인했다.
- 기존 Native State33행, 앞단2개,7/8 확정2개, 비동기2개, 실제 Client Renderer 접합을 재대조했다. 기존 fixture를 수정하지 않았으며 카메라16/미션 수명831행을 이번 재실행 수에 더하지 않는다.

Release 빌드1회 종료0·경고/오류0이다. 관련 보호7입력(AGENTS/두PE/TAFF/기존fixture3개)은645 기준과 일치하며 전체2632개/기존fixture773개 재감사와 구별한다. 원시 근거는 이 PC의 `extracted/cpp-mission-starting-parent-660-*`다.

실제 게임/GUI·CD 열거·새ZIP·전체34·신규MP 검사는 없고 주 검사 EXE도 재빌드하지 않았다. 두 World 교체의 근거는 deferred World이며 완전히 로드된 GUI World의 정리/CD/음악/안내 연쇄를 새로 실행한 근거로 확대하지 않는다. 사제 포획/희생·특정 적 공격·승리·공중3종 전용 행동 검사는 하지 않았다. 더 많은 기능 묶음 뒤 필요한 스테이지 통합을 몰아서 수행한다.

## 잔여

단독 정상 미션의 최초 복사/로그·기존 World 반납·새 후반·7/8 확정을 같은 부모에 연결했다. current+84 재접속의 SavedGame/파일 재선택/진행 문구, ZonePending0의 비로컬/리스너 부재 분기, 메타맵 Poll의 실제 장치·`004c1e70` 시계 패킷 등록·외부 동기화/주기 내보내기는 남는다. 전체 State7 분기나 GUI 동일성을 완료로 세지 않는다.

State1/연속 전환·나머지 UI/Edit/시나리오·호환성·BaseFile 소비자/ABI·시작 레지스트리·메타State19/LeaveBattle·후순위MP 무시험 사전 구현과4~7차를 계속한다.10.72 전체 State7/실행의 새 근거는 없다. 자연 완주/전체 동일성은 사용자 확인 대상이다.
