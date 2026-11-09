# 제품1236 — Factory 정기 처리와 현재 후처리 수명 연결

**제품1236의 구현과 허용 검증을 마감했다. 전체 목표는 미완료다.**

Factory/Outpost의 실제 `00509148/00509210 +5c=004549c0` 두 표를 같은 `RawFactoryRegular`에 연결했다. Native Session과 NoWorld Host가 Storage의 현재 TYPE/프레임·실제 가상80·같은 게임 난수를 공유한다. 현재 레벨→현재 첫 프레임→다시 현재 레벨→현재 개수→난수→프레임 설정 순서와 DWORD 감김을 유지한다. 원문4558c0의0범위 나눗셈은 같은 실제 난수를 한 번 소비한 다음 예외로 남긴다. 사건4의 비0 count는 다른 장치를 읽지 않고0을 반환한다.

큰 Begin/Finish와 플레어만 약한 현재 Runtime 대여→실제 전체 Runtime pin→함수 사본→기존 전체 처리기 호출이다. 기존 RawFactoryUpgrade의 예측 그림/현재 레벨·소유자/건설·Depend쌍·완료·AI통지를 재사용했다. 그림 생성은 원문4af530의 실제 Factory.Create, 소유자 설정은 실제 가상74, Depend 역할은 현재 공유 GameAux를 사용하도록 바로잡았다.

Factory 가상20도 기존 RawFactoryPostPop을 Storage가 장기 소유하도록 옮겼다. Native 자산 묶음은 같은 본문을 빌리고 typed 경로는 기존 로컬 소유를 유지한다. 실제 공통 후처리/캐리어 부모·Host 예약·현재 두 타이머 역할/Outpost 프레임 역할·거주/Outpost목록·중복 삭제/경고·영역 owner를 공유한다. World 반납 뒤 기본 분배도 같은 본문으로 돌아간다. Outpost의 구현/컴파일은 포함하지만 전용 실행 검사는 하지 않았다.

원본 x86 독립 fixture는656행이며128행은 실제 DIV0 예외다. 실제4549c0 분배/4ad0f0·4ad0a0/4558c0을 실행하며 TYPE/프레임·기본/플레어 장치와 큰 Begin/Finish의 묶음을 명시 경계로 둔다. **큰 업그레이드의 모든 명령을 이 fixture에서 실행했다는 뜻은 아니다.** 해당 본문은 기존 RawFactoryUpgrade의 비MP6검사와 별도 일반 UI 업그레이드로 확인한다.

새 순수2검사·장기6검사와 기존132·업그레이드 비MP6을 합친146/146이 통과했다. 첫 검사143/146의 기록을 보존했다. 무효과 trace의 빈 문자열과 fixture의`-` 표기를 맞췄고, 새 후처리 검사가 옛 독립 사전검사 Activate 대신 실제 제품의 Pop→Activate→가상20 경로를 사용하도록 고쳤다. 최초 예약은 일반 건물의 실제 가상30 관문으로 확인하며 원문 기대값/fixture/제품 관문을 약화하지 않았다.

현재 장기 Regular 연결은70/87VT, 잔여17VT/고유15함수다. Pre/Post·통지83/83과 전체 NoWorld/큰 AI/배치/Fort 상위 Finish·전수 caller/owner 감사의 완료는 구별한다. 다음 단위는 Residence `0050f540/0049a4e0`다. M6-l/m·M7/M8·한국어/시나리오/버그16상위계획은 남는다. 포획/희생·특정 적 공격·승리/자연완주·공중3종 전용·MP·사용자 직접 확인 실행 제외를 유지한다.

디컴파일을 새로 하지 않았다. 현재 호스트 HJOW-Athlon은 Client양판본2026-11-03, R2026-11-07의 마지막 산출 호스트와 같다. 원시 증거는`extracted/cpp-resume-1236/`에 보존한다.

최종 Release/x64 제품2·집중1대상 빌드는29.756초/종료0·경고/오류0이며 소스/새 근거17개를 고정했다. Tutorial4 생산·건설 요청 뒤 실제TYPE75/owner1 생성과600프레임 대기, Dissolved Alliance의 기존TYPE86/owner1 워크샾 UI 업그레이드(레벨1→2, SP4000→3000, 예측그림 생성/제거,1200프레임 대기)를 확인했다. 두 실행은 정상 종료0이며 PNG를 직접 검토했다. 원본/CD/기존fixture·AGENTS2,806파일과 실행 전후 제품/격리 설치 자료가 불변이다. 빌드·집중·GUI 작업은 모두 종료했다.

- Netstorm.exe: 14181376 bytes, SHA256 `5901b5663b3e6a1f01c22ee801968c3ccad9f63d4ef140d6a993073f8f3612ad`.

- R.exe: 677376 bytes, SHA256 `20fd654726ad6d25d403e586571b938cd1a7201eebd47a13065cb02ecb1800c2`.

[확정 보고서](../../cpppj/factory-regular-owner-20261110.json).
