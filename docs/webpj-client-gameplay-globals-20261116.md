# ClientGameplayGlobals 장기 상태와 Runtime 수명 (WB04-E570)

기준선 `225553df`의 `client/ClientGameplayGlobals.cpp`83줄·헤더175줄(총258줄)을 이식했다.
대상과 EnergySupply 소스의 기준선 뒤 변경은0이다. cpppj·보호 원본·공통 인계·dotnetpj는
수정하지 않았다. E569에서 확인한 선행 커서/선택·SID 진단·지형 캐시를 실제로 조립한다.

## 이식과 수명 계약

- 실제 PlayerWorldState·CursorDropState·ClientUserSelectionGlobals·ContainedFinderState와
  EnergySupplyState를 장기 소유한다. EnergySupplyState의 현재float 제곱반경484와 여섯DWORD는
  기존 RawEnergySupply 모듈에 추가했다. 기본/일반 fresh의 동맹1과 사용자 치트 보존을 구별한다.
- 공급된 Authority/GameAux/Subscriber를 같은 객체로 빌리며 없으면 기존 독립 입력을 유지한다.
  두 Subscriber 상자와 현재 권한·checking, GameAux의 네TYPE 별칭은 사본으로 고정하지 않는다.
  실제 AiDebug·Camera·CursorGlobals·두 지형 캐시·SID 진단·옵션87BYTE·반경4float·기한9double은
  Client 수명에서 한 번만 생성한다. Bitmap 분류와 Stamp99 변형 캐시는 서로 별개다.
- 헤더의 장기 참조22칸을 같은 `{value}` 상자로 노출한다(DWORD/signed DWORD/float32).
  특별 모드 보조DWORD, 실제 Debug 주소식별자8종 및 기존 AiDebug7로그를 현재 owner에 연결한다.
  주소는 식별자이며 호스트 메모리를 역참조하지 않는다. 서버 포트 값 보관이 외부 접속을 만들지는 않는다.
- Acquire는 중복을 먼저 거부하고 같은 Player/커서/검색/에너지/선택 객체를 제자리 fresh로 바꾼다.
  특별/사용자 치트·커서 기록기·선택 누적과 장기 상태는 보존한다. 실제 Native 정리/prefix 전체로
  세지 않는다. AcquirePrepared는 상태를 쓰지 않고 같은 물리를 빌린다.
- 명시 release는 C++ Use 소멸자다. 일반 반납은 공개 대상을 닫고 PlayerKnown/커서/editor만
  무효화하며 prepared 반납은 Runtime 공개만 닫는다. 중복 release와 낡은 대여는 새 세대를
  건드리지 못한다. Publish/Withdraw는 같은 현재 Session만 선택하며 미연결/중복/다른 대상을 거부한다.
- BindRuntimeOwner는 실제 전체 owner를 WeakRef로 공개하고 Session의 bindRuntimePin 입구에
  현재 Session을 캡처한 함수를 설치한다. PinCurrentRuntime은 현재 Session/owner 일치와 weak 수명을
  확인한 뒤 전체 owner를 강하게 잡는다. 호출 수는 C++64비트 size_t에 맞춘 BigInt이고 중첩 pin을
  구별한다. caller는 try/finally에서 release해 호출 수를 먼저 줄이고 전체 owner 참조를 놓는다.
  배송 중에는 owner 재공개와 Withdraw/State반납을 거부한다. 이전 pin 함수도 다음 호출에서는
  같은 Session의 최신 전체 owner를 읽는다.
- C++ RawCampaignSession friend의 WithdrawForState는 `withdrawGameplayForState(use, session)`
  모듈 브리지로 분리했다. 실제 대여/현재 Session/배송 관문만 담당하며 자식 정리 증빙을 만들거나
  자식을 정리하지 않는다. 실제 Session 소멸자가 정리 증빙을 확인한 뒤 호출하는 결선은 D/E 몫이다.

## 검증

신규16건을 추가했다. 첫14건 중1건은 finder 진단의 실제 오류 문구와 다른 정규식 때문에 실패했다.
기대 문구를 기존 `dead 부모 종속 조회 assert`로 교정했다. 이 첫 교정은 시험 정규식이다.
현재 owner 교체와 JS 빈 Session 거부 검사를 포함한 최종 집중45건
(신규 owner16·기존 CursorGlobals/E569 Cursor·EnergySupply·MissionStart)이 통과했다.

마무리 검토에서 아직 공개되지 않은 현재 Session(null)과 전달된 null이 같아 반납이나 State 정책
변경으로 들어갈 수 있는 JS 경계를 발견했다. C++의 유효한 참조 계약에 맞게 Withdraw/State/Bind/Pin에서
빈 대상을 거부하고 호출 관측도 false로 유지했다. 실제 빈 입력 거부 뒤 일반 반납의 PlayerKnown
초기화가 계속됨을 추가 검사했다. 보강 전 전체 Node22/26의 각5,399건 결과와 별개로 최종 코드의
전체 검사를 다시 실행해 두 버전 모두5,400건 통과·실패/건너뜀0을 확인했다.

- 보호10.78 PE22칸·4float 선택표·9double 기한·에너지 반경을 파일 바이트로 직접 대조했다.
  원본 SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다.
  기존 manifest의 선택 자료 정책은 보존하며 없는 환경에서는 PE 대조만 진단과 함께 생략한다.
  현재 작업 환경에서는 모든 대조를 수행했다.
- 실제 같은 장기 참조/Subscriber·권한/보조 별칭과 폭, 일반/준비 대여·반납의 각 보존/무효화,
  중복 선행 거부·부분 구성 반납·낡은 대여·현재 공개/해제 순서를 확인했다.
- E400/CursorLifecycle_CurrentClientCheckingSurvivesReset의 실제 ClientGameplayGlobals
  Acquire/AcquirePrepared 부분을 재대조했다. 같은 현재 finder의 dead 진단도 효과가 바꾼 checking을
  다음 호출에 읽는다. 기존 수명 몸체의 선택된 누락 진단 검사는 원본 fixture 회귀에서 계속 수행한다.
- CampaignSharedDiagnostic의 현재6·두 미션 제한/DrawWater·카메라/캐시/옵션/기한 보존,
  실제 RawMissionStart의 순차 제한DWORD 쓰기와 SP 효과 뒤 현재값 유지도 확인했다.
- 실제 RawSurfaceWorld를 가진 최소 Session과 전체 owner로 중첩 pin·현재 owner 교체·예외 finally
  반납·배송 관문을 검사했다. 별도 Node의 실제 GC로 마지막 pin 전에는 owner가 살아 있고 반납 뒤
  weak 수명이 끝나며 다음 pin이 실패함을 확인했다. GC 자식은 현재 검사와 같은 Node 버전이다.

집중 coverage: ClientGameplayGlobals 줄100%/분기97.37%/함수100%, EnergySupply 줄100%/
분기90.85%/함수100%. 로그는 `extracted/wb-e570-focus2.log`(Git 제외)다. 64비트 호출 수 소진처럼
현실적으로 만들지 않은 분기를 실제 실행했다고 세지 않는다. 최종 전체 검사는 Node v22.23.3·v26.13.0
각5,400건 통과·실패/건너뜀0이다. 로그는 `extracted/wb-e570-node22-final.log`·
`extracted/wb-e570-node26-final.log`다. 대응표1,160행 오류0, port838·partial33·todo133·제외156,
session/oracle0을 확인했다(`extracted/wb-e570-port-map.log`, 모두 Git 제외).

manifest는 기존801항목·실제 cpppjBasis를 유지하고 original-pe-data 소비자/모듈 및 변경 모듈의
추적 이력만 갱신한다. 기존 EnergySupply1058관측(파일1059행)의 정규화SHA-256은
`d020d2165be81e8db054c63804976ddd9a50876e3765243e7f7fcc38f2299802`, EnergySort567관측(568행)은
`58f67ad1e9207c2b7793101100855d504c6167de92ee0176adf3be816489ab4e`다.
새 원본 관측·게임·브라우저 실행은 하지 않았다.

## 남은 통합과 다음 작업

현재 부품 port이고 앱/Session 설치는 아직 없다(session/oracle0). CampaignSessionTests의
실제 Client·Session 생성/Load·외국 Subscriber/부분 생성 실패·실제 World pin 전달·자식 정리
증빙 소비와 전체 외곽 GameWorld 반납은 D/E에서 재대조한다. 최소 Session은 실제 게임 행동/Load를
성공으로 대신하지 않으며 bindRuntimePin 입구만 명시한다.

후속 생성자 감사에서 `RawCampaignSession.cpp:1208`이 SoundProcessSystem을 무조건 만들고
사제 shield에 SoundList.Lookup→Sound47.Add를 연결함을 확인했다. 다음은 이 SoundProcess361줄과
같은 Sound.cpp의 SoundList 공개 경계를 8단계에서 앞당기는 작업이다. C에 필요한 항목을 더하고
원본 soundprocess fixture와 실제 Host/보호막 수명을 대조한다. 브라우저 재생 장치/전체 SoundPlayer는
그 자체의 후속 적응 작업이며 성공 함수로 대체하지 않는다.

그 뒤 D 구획 RawCampaignSession1(구성·소유 상태·Load)을 공개 경계로 나누어 연결한다.
ClientGameplayGlobals와 필수 사제/피격/Archer/Motion/Path/Cursor는 갖춰졌다. 같은 장기 owner와
자산/보충 표·현재 참조부터 구성하고 실제 Fort 설치/Load를 이어 붙인다. A의 World 정리/선택 반납과
B의 Client 서비스는 실제 Session 허브 뒤에 연결한다.
