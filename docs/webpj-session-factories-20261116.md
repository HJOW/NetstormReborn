# Session의 실제 Factory 연구·업그레이드·입력 (WB04-E599)

`225553df`의 `RawCampaignSession.cpp`3157..3176과
ConnectFactoryInput/Upgrade3178..3192를 같은 owner에 연결했다.
E596~E598 뒤 InstallContents의 마지막 구간이며 생성자1650의 조립을
마쳤다. 다음은1651 ConnectFlare다. 해당 cpp와 사용한 기존 네 Factory
몸체의 기준선 뒤 변경은0이다. Native 장기 Storage는 후속 Client 결선이다.

## 같은 실제 생산·HP·Host와 현재 상태

- 연구는 같은 내용물 생성/Carrier 검색/생산 슬롯·Refresh·가상 삭제와
  현재 도움말/슬롯 로그/요청 송신을 사용한다. 송신 채널은 signed32,
  슬롯 로그는 현재 owner BYTE를 읽는다. Flare와 업그레이드 수신기는
  호출 때 후설치 포인터를 고른다. 로딩/dirty·권한/시각/Outpost TYPE는
  현재 같은 상태이며 dirty 쓰기도 공통 장부로 간다.
- 업그레이드 명령의 비용/SP는 실제 hp다. SharedRegular 예약은 실제
  미부착 RegularProcess를 확보하고 같은 객체에 생성 인자를 채워 같은
  Host에 한 번 부착한다. 부착 뒤 현재 시각으로 ResetTask한다. 예약
  누락/부착 실패는 오류이며 원문처럼 새 예약이 이전 미부착 예약을
  교체한다. JS VM의 복구 불가능한 OOM은 모사하지 않는다.
- 요청/방송·로그는 현재 실제 서비스와 기존 Broadcast를 사용한다.
  진행은 현재 두 Factory VT의 Level 관문·같은 HP/표시·소리·생성/소유권/
  Pop·건설 준비/시작·의존 Form/가상 삭제를 연결했다. 표시에는 현재
  물리 span을 빌려 효과 사이 슬롯 사본을 정답으로 쓰지 않는다.
- AI 여부는 현재 owner BYTE와 Player+b0 DWORD를 읽는다. 완료 통지는
  도전 모드/owner signed 비트 관문 뒤 현재 토큰이 있을 때 같은 후속
  AI 생산 부모로 간다. 후설치 AI/Flare/건설을 다른 몸체로 대신하지 않는다.
- 입력은 같은 Level/프레임·비용/분산 SP·업그레이드/연구로 간다.
  미지 명령의 공통 객체 위임에는 원문처럼 packed 인자를 넘기지 않는다.
  editor는 원문의 globals.editor이며 ownerMode.fort로 대신하지 않는다.
  원문 stderr 진단은 console.error로 연결했다.
- 생산 완료 뒤 한 번 실행하며 구성 자체는 연구/생산/업그레이드 요청을
  하지 않는다. 부모는 World 프로세스를 정리한 뒤 미부착 예약을 반납하고
  자기 closure를 닫는다. InstallContents 완료는 World/Session 게시가 아니다.

## 최초 실패·근거·검증

새 전용 원본 관측은 없다. 보호 자산/실제 모듈·명시 소비자 계약을
대조했다. SharedRegular 부착/시각 재설정, 실제 내용물 생성·검색과 SP
증감은 기존 몸체를 실행했다. 후속 포인터의 함수 대역은 인자/분배
계약만 확인하며 실제 AI 행동·업그레이드 완주 증거가 아니다.

신규12건 첫 실행은11건 통과/1건 실패였다. 검사에서 Gun의 checking을
signed -2로 예상했으나 실제 현재 Gun DWORD는4294967294였다.
기존 비트 표현을 그대로 단언하도록 검사를 고쳤고 제품을 완화하지 않았다.
신규12건을 포함한 **관련92건이 모두 통과**했다. Node v22.23.3·v26.13.0 최종 전체는 각각 **5753건 통과, 실패·취소·건너뜀0**이다. 대응표1160행/오류0도 확인했다. Node24는 사용자
지시로 생략하며22/26 통과 시 호환을 가정한다.

직접 연결된 manifest 항목은0개라806항목을 유지했다. Source 계약을
원본 관측으로 새로 등재하지 않았다. JS130994행·검사769개이며,
대응표 상태/잔여와4-D36/51·session0은 그대로다.

로그: `extracted/wb-e599-initial.log`, `wb-e599-focused.log`,
`wb-e599-port-map.log`, `wb-e599-node22.log`, `wb-e599-node26.log`.

## 다음 시작점

생성자1651 ConnectFlare → Construction/Damageable/IceTower/
CarrierSteal/BombPostPop/AcidZap/AcidFence →1659 현재 CarrierRegular
훅·이동/건물과 나머지 모듈로 이어간다. World.BindRuntime1825·상위
Runtime 공개·Load/Login·프레임·L2/L3는 여전히 남았다.

cpppj/보호 파일 수정·디컴파일·게임/브라우저 실행·자연 완주·실제 포획/
희생·특정 적 공격/승리·공중3종 전용 관측은 하지 않았다.
실제 미션은 아직 플레이할 수 없다.
