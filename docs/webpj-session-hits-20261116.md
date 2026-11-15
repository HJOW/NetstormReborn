# Session 피해 상태·AI 통지·Hit 효과 (WB04-E581)

`225553df` RawCampaignSession.cpp1340..1397을 E574~E580의 같은 기반 owner에 연결했다.
피해 상태 getter→AI 피격/Player 통지→효과 기본값→RawCampaignHitBindings 순서다.
전체 Session은 계속 **partial**이며 GunHit/투사체·나머지 생성자·외곽 공개·Load/로그인/
프레임은 잔여다. 이 구간을 전체 교전 또는 앱 미션 플레이 완료로 세지 않는다.

## 연결 범위

`connectHitBodies()`는 총/커서 생성 구간 뒤 한 번 실행한다. 사제 보정 생략은 현재
missionPost를, 일반 피해는 현재 Server/로컬 owner BYTE·같은 Player+78 주 섬을 읽는다.
원문666의 GunImpactState도 같은 LocalState.loginEnd double 상자를 빌린다.
실제 충돌 효과 생성은 그 현재 기한을 읽어 억제되며 지역 double 사본을 동기화하지 않는다.

AI 피격은 같은 RawAiBrainMemory/Pool/현재 TYPE을 사용한다. Player 통지는 같은1620BYTE
기록+0xb0의 현재 AI 토큰을 읽고 실제 RawAiDamage에 보낸다. AI0/중립/challenge의 자연
반환은 기존 실제 몸체가 판단한다. 로그는 현재 TYPE 이름·0방향 절삭된 signed 좌표·
brain+260의 signed 번호를 원문 서식/두 LF로 출력하며 현재 brain+b00이0이면 출력하지 않는다.
비행 홈은 후속 실제 Flyer 모듈에 연결하며 이번 새 검사에서는 해당 파생을 실행하지 않았다.

Hit 효과는 C++ 지역 값 인수처럼 다섯 하위 묶음을 복사한다. 사제 defaultFrame은 원문대로
제공 여부와 관계없이 같은 현재 projectileTypes 행으로 덮어쓴다. 소유자 통지·벽시각·섬
피격 소리/Tell·Burning 진단/생성/불프 위치 소리·신전 건설 기한/표시·Dude Burning/실제
현재 Path 끝점과 시작은 제공 장치를 보존하고 누락 기본값만 연결한다. 경고 이벤트는
현재 missionPost가0일 때만 보관한 remove 함수를 부른다.

피격 Bindings는 같은 World·사제·HP·게임 난수·속도 배열과 실제 장기 거주 목록을 빌린다.
`assetState.currentResidences()`로 빌린 목록을 사용하고 지역 빈 목록을 대신 읽지 않는다.
Particles/Motion/PrepareConstruction/Building은 후속 생성자가 채울 실제 슬롯에서 조회하며,
선택 시 아직 설치 전이면 명시 오류다. 가짜 성공 몸체를 만들지 않는다. 부모는 현재 World
프로세스를 먼저 정리한 뒤 이 연결 단계를 무효화한다.

## 서비스 복사 교정

원문990/994의 Impl은 `RawCampaignSessionServices input`을 값으로 받아 `services(std::move(input))`
에 보관한다. CampaignAiServices와 RawCampaignHitEffects도 값형 구조다. 기존 기반 owner의
AI 묶음은 호출자가 넘긴 객체를 그대로 빌리고 있었다. AI와 Hit의 각 하위 묶음을 복사하도록
보완했다. 외부 함수가 캡처한 실제 장기 장치는 그대로 유지한다.

기존 E575 진단 검사는 호출자 AI 묶음의 재대입을 Session의 현재 장치 변경으로 취급했다.
이번에는 호출자 재대입이 Session에 미반영임을 먼저 검증하고, 실제 `o.services.ai.assertion`
칸을 바꿔 현재 진단 B/생성 시 캡처한 HP 진단 A를 그대로 구별한다. 기존 기대 A/B 순서는
약화하지 않았으며 실제 소유자와 값 사본을 구별하도록 입력을 교정했다.

## 근거와 검사

Session·CampaignHitBindings·RawAiDamage·RawPlayerDamageNotice·RawGunImpact의 소스/헤더는
기준선 이후 HEAD 변경0이다. cpppj 수정은 없다. 기준선 생성자와 실제 하위 소비자/기존
CampaignHitBindings 계약·AI 피해149행 원본 fixture의 본체/목록 계약을 대조했다.
기존 원본 fixture 검사는 전체 회귀에서 유지한다. 새 원본 관측/fixture는 없으며 manifest
803항목을 유지한다.

신규12검사는 실제 보호 자산·World/Core·Client 전역·Host·AI 메모리에서 다음을 확인한다.

- 앞 단계/반복/반납·같은 실제 다섯 피해 몸체/Pool·미공개 Runtime.
- 현재 보정/Server/로컬 BYTE/주 섬·같은 double 기한과 실제 anim 생성 억제/종료.
- 실제 일반 HP DWORD 변경·Player AI0의 자연 반환.
- 실제 사제 HP 변경·같은 회복 Regular 두 사건 취소·TYPE 기본 프레임.
- 실제 지면/주 섬에서 현재 미션 경고 억제·기본 소리/Tell·현재 벽시각/경고 기한.
- 제공 Hit 함수 보존·묶음 값 복사·사제 기본 프레임의 무조건 덮어쓰기.
- 중립/AI0/challenge와 현재 잘못된 AI 토큰의 실제 메모리 검증.
- 실제 AI 본체/미움·매우 미움 목록 append와 현재 이름/번호/좌표·정확한 로그.
- 로그 누락 전에 남은 실제 미움 append와 아직 수행하지 않은 매우 미움 append.
- 현재 AI 진단/제공 함수 사본·같은 SoundList의 불프 위치 소리와 선택 오류.
- 미설치 Particles/Motion/Construction·현재 건설 번호와 경고 기본값의 선택 실패.
- 실제 장기 거주 목록 오류 앞의 AI/상태/효과 접두와 반복 거부·대여 반납.

최초 신규12검사는10통과/2실패였다. 시험의 부분 sound 묶음이 선행 SoundProcessSystem의
필수 효과를 없애 Hit 구간 전에 거부되었다. 온전한 선행 소리 장치를 구성하고, 누락 경계
검사에서만 앞 구간 뒤 실제 현재 서비스 칸을 바꾸도록 교정했다. 제품 검증은 약화하지 않았다.
두 Node 버전의 집중97검사는 모두 통과했다. 신규 모듈 줄97.73%·분기88.89%·함수93.55%다.
미실행 후속 건설 기한/Path 성공 경로를 억지 대체 객체로 덮지 않았다. 로그는
`extracted/wb-e581-focus2-node22.log`·`extracted/wb-e581-focus2-node26.log`(Git 제외)다.

전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각5,528건 통과, 실패·취소·건너뜀0이다.
로그 `extracted/wb-e581-node22.log`·`extracted/wb-e581-node26.log`는 Git 제외다.
대응표 검사1,160행/오류0(`extracted/wb-e581-port-map-pre.log`)이다. port841·partial34·todo131·제외154,
session/oracle0·단계28/43은 유지한다. AI 본체/목록은 실제 memory owner에 설치한 고정
수치 입력이며 AI 생성/미션 Load 완료 증거가 아니다. 원본/클론 게임·브라우저·오라클·
디컴파일·공중3종 전용 생성/행동·새 원격 멀티플레이 실행은 하지 않았다.

## 다음 시작점

1398의 GunHit→ProjectileGeometry→FenceTarget→최대 검색 범위/GunBlocker부터 이어간다.
GunHit의 Rain ApplyFlyerHit(2374)와 현재 Flyer/Form/방송의 후속 연결을 구별한다.
이후 나머지 모듈/등록·외곽 공개·같은 Template TypeNames→Objects/Contents·Load·로그인·
프레임/종료가 잔여다. L2/session-dump·실제 미션 화면/자연 플레이는 미검증이다.
