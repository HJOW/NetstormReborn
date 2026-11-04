# cpppj 단위890 — 주문 효과 최초 예약과 가상20 분배

2026-11-05 / 정밀 디컴파일 최종 호스트 **HJOW-Athlon, 2026-11-03**. 원본/디컴파일 산출물은 읽기만 했다. cpppj 전체 목표는 진행 중이다.

## 원문과 구현

보호 10.78 PE의 recipe82개와 공통501dd0, 총83개 가상 표를 직접 다시 읽었다. +20은 **46종**, +30은 전부004ad470이다. 889 문서의 “43개가량”은 개략 수치였고 여기의46종이 정확하다. 현재 C++ 주소/분배와 대조해 세 주문 효과 및 예약TYPE187의 누락을 찾았다.

| 가상 표 | +20 | 구현 |
| --- | --- | --- |
| 00502c38 / TYPE174 GravitationEffect | 0041f870 | 중복event1 없음/new28 성공이면 고정float0.05로Regular 예약 |
| 00502cf0 / TYPE176 HealEffect | 0041f920 | 같은 관문 뒤455930(0,float0.05)+**double0.05**를float로 저장해 예약 |
| 00502db0 / TYPE177 MeteorEffect | 0041fab0 | 같은 관문 뒤455930(float0.01,float0.05)를float로 저장해 예약 |
| 00503ff0 / TYPE187 reserved | 0042abb0 | 크랩00503f20과 같은 기존 후처리 몸체로 분배 |

`RawBombPostPop`은 표시TYPE/free/contained를 추가 관문으로 쓰지 않고 실제VT만 고른다. flags1→현재event조회→실제 메모리 확보→필요한 난수→같은 확보 메모리의생성/부착 순서다. 그라비테이션·중복·할당실패에서는 난수를 소비하지 않는다. 뒤 생성 장치가 실패해도 앞서 소비한 난수는 되돌리지 않는다. 하위 조회가VT를 바꾸어도 이미 선택한 몸체를 이번 호출에 유지한다.

Session은 같은 장기 `ClientRandomGlobals::Game()`과 실제Host `FindEvent`/`RegularProcess`/Kernel을 연결한다. Session 해제 전에 공통 분배의 접두 포인터를 반납한다. 상위 `SquidPostPop::DispatchPostPop`이 접두 다음 공통 장부를 한 번 호출하므로 공통 깊이를 중복 감소시키지 않는다. TYPE187의Halt는다른주소이며이번지원주소확장이Halt/포획몸체를바꾸지는않는다.

10.72는 기존171TYPE 계약을 유지한다. 이번 세 효과의174/176/177 주소를CD에 억지로 대응시키지 않았고, 새 접두는10.78 전용이다. 공통예약조회004afb40의CD 대응은004ac890이며 다음 현재TYPE 감사에 함께 기록한다.

## 83표 정적 분류와 Validate

현재81표는 C++ 실제분배에대응하며,0050c9f0(TYPE108/109)와0050cc08(TYPE161)는8번MP메타맵잔여다. 81에는공중3종과Outpost의**코드만대조한4표**가포함된다. 전표후처리/전체Pop 실행완료나기능동일성의증거로세지않는다.

실제PATCH `Pop`은입구에서`PopPatch`로돌아가며`RunSquidActivate`→`PostPopVirtual`→현재파생몸체로이어진다. `PostPopVirtual`/`PostPopBase`의PATCH는기존명시Validate를호출하지않는다. `SquidPop::Pop`후반의Validate/Activate caller는PATCH조기return뒤의CD본문이고독립`SquidPostPop::Activate`도명시사전검사계약이다. 해당레거시API의존재를실제PATCH전수사전검사로판정하지않았다.

## 검증 결과

- 원문41f870/41f920/41fab0과난수455930의전체코드를격리실행했다. 432입력×x87 027f/037f=864관측및동일864재관측에서호출열/float payload/최종공유시드가일치했다. FS:0정상복귀,비휘발레지스터/ret4/스택canary/raw불변도검사했다.
- 중복조회4afb40/new28/예약생성496e80/공통4b0d30은명시효과경계이며,원문하위전체실행/실제시전효과라고쓰지않는다. C++은실제호스트를별도로연결한다.
- Release 제품/두 집중 대상 빌드 **952.728초, 종료0·경고0·오류0**. 새4/고유24사례를7필터로 각각 한 번 실행해 **첫 실행 전부 통과·실패 단언0**이었다. 컴파일/검사 입력 교정이나 재빌드는 없었다.
- 기존 보호2769파일/949fixture 불변, 새fixture1개 후2770파일/950fixture다. 빌드 입력14개는 빌드·검사 중 불변이었다. 전체71/CTest 결과로 확대하지 않는다.
- 제품 **890/11.01**, 10,670,080바이트. EXE SHA-256 `d35a7534f476852bb8cc475d3f771c8edf0432fe13e5ad3e456743dfcc7d0f63`.
- 기존`CarrierSteal_LazyBoundariesAndPacketBounds`의503ff0불지원단언만PE근거로지원으로교정했다. 패킷을포함한해당기존검사는실행하지않고새순수지원주소사례만실행한다. 기존fixture입력/기대값은변경하지않는다.

## 잔여 및 인계

checking10=부모4+부분3+MP3/완료부모38·부분3을유지한다. 다음은같은Pop의실제진입/남은현재대여와Host.FindEvent의Regular현재TYPE00541128/00541164경계이며,다른mode/공통Pop전체·군도/Factory/State/파일/객체UI/Edit/Windows/무검사MP및4~8차도남는다.

게임/GUI/공격·포획·희생·승리/신규MP·패킷·통신/공중3종전용/전체71·CTest·34미션·ZIP는실행하지않았다. 이번예약검사는전투/AI/시전Kernel을진행하지않는다.

증거: [JSON](../../cpppj/bomb-postpop-dispatch-20261105.json). 로컬원시는`extracted/cpp-resume-890/`이며다른PC에있다고가정하지않는다.
