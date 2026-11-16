# Session의 실제 내용물 통지 접두 (WB04-E596)

`225553df`의 `RawCampaignSession.cpp`3020..3049 중 독립 typed 분기를
같은 Session owner에 연결했다. 생성자1650의 `InstallContents` 내부
첫 구간이며, 다음3050의 ContentFormHost/수명·운반·생산과 전체 생성자는
아직 미완성이다. 해당 cpp 및 사용한 기존 몸체의 기준선 뒤 변경은0이다.
Native 장기 Storage 대여는 실제 Client 결선 때 이어간다.

## 같은 World와 현재 상태

- 실제 `GumpFormHost`를 같은 Pool/Factory/Destroy로 만들고 기존 Form
  분배기에 등록한다. UI 통지는 호출 때 현재 서비스를 선택한다. Gump의
  소유 ID0은 기존 몸체가 자연 생략한다. fallback도 같은 분배기를 빌린다.
- `unitLost`의 빈 칸은 원문 FillMissing3025의 선택된 오류로 채운다.
  같은 `hp`·Runtime 삭제 훅·`MakeCampaignDeletionEffects`를 World의
  공통 삭제에 연결하고 같은 삭제 상태의 `silentType`을147로 쓴다.
  첫 객체 이후 설치는 기존 World 계약대로 실패한다. 앞선 Gump 등록은
  남으며 해당 초기화를 재시도 성공으로 숨기지 않는다.
- Player 통지 서비스 표를 C++ 값형 인수처럼 복사한다. money는 같은
  SP 저장소의 Get 결과 DWORD를 float로 bit_cast하며 입력의 money를
  사용하지 않는다. SP 추적 커서/첫 배열/첫 순열은 기존 실제 조회기를
  쓰고, PatchDiagnostic/현재 표본/남은 검사 횟수·PlayerKnown·battleOwner·
  Gump TYPE·UI 사건은 현재 같은 상태를 읽거나 쓴다. PlayerKnown을
  전투 모드로 대신하지 않으며 SP Get 뒤 표본 재조회도 기존 순서다.
- `RawContentParentNotice`는 현재 부모 VT의 실제 빈 본문/파생을 고른다.
  파생은 호출 때 같은 후속 lifetime 포인터로 간다. 아직 설치되지 않은
  lifetime은 선택된 위치에서 명시 오류다. 이번 단계에서 별도 수명이나
  임시 Player 분배를 만들어 성공시키지 않는다.
- ContentForm의 현재 containedType/battle/checking 조회와 현재 AI 진단
  서비스를 연결했다. 원문처럼 진단에는 표현식만 넘긴다. 이는 후속
  ContentFormHost가 그대로 빌릴 상태이며 아직 Host6 등록은 하지 않는다.
- 같은 Pool·현재 로컬 소유자·장기9칸 기한 배열의 `RawPriestTakenNotice`를
  구성했다. 참여/벽시각/이름/전역 음향/Tell은 현재 실제 서비스를 고른다.
  이름은 같은64BYTE handle의 첫 NUL 앞 바이트 문자열이며 UTF-8로 재해석하지
  않는다. SoundList 조회 뒤 전역 play의 원문6인자를 보존한다. 구성 자체는
  포획 통지를 실행하거나 장기 기한을 초기화하지 않는다.
- `connectContentNotices`는 앞 Runtime 훅 완료 이후 한 번 실행하며,
  반납 때 자기 closure를 무효화한다. `noticesReady`는 이 접두만 뜻한다.
  World.BindRuntime1825/외곽 Session 공개/Load는 여전히 실행하지 않는다.

## 근거·최초 실패·검증

새 전용 원본 관측은 없다. 보호 자산/실제 기존 모듈과 명시 소비자
계약을 사용했다. 포획 통지의 하위 훅 검사는 인자·선택된 실패의
계약 검사이며 실제 사제 포획/해방 장면이나 게임 플레이 증거가 아니다.

신규13건 첫 실행은12건 통과/1건 실패였다. 검사에서 존재하지 않는
Storage.deletionState를 비교한 오류로, 제품의 같은 World 상태를
연결 전후 비교하도록 고쳤다. 최종 신규13건을 포함한 관련66건은 모두
통과했다. 같은 전역 값/SP 부작용 뒤 재조회, 함수 값 복사와 현재 서비스
교체, 실제 Form 등록, 부분 실패의 앞선 효과와 미공개 경계를 대조했다.

Node v22.23.3·v26.13.0 최종 전체는 각각 **5714건 통과, 실패·취소·건너뜀0**이다. Node24는 사용자
지시로 생략하며22/26 통과 시 호환을 가정한다. 대응표1160행/오류0,
JS130695행·검사766개다. 직접 연결된 manifest 항목은0개라806항목을
유지했다. 새 계약 검사를 원본 관측으로 등재하지 않았다. PORT_MAP의
Session 행은partial이며 port845/partial37/todo125·잔여162/34373행,
4-D36/51 및 session0은 그대로다.

로그: `extracted/wb-e596-initial.log`, `wb-e596-new.log`(교정 전 재실행),
`wb-e596-focused.log`, `wb-e596-node22.log`, `wb-e596-node26.log`,
`wb-e596-port-map-initial.log`.

## 다음 시작점

`InstallContents`3050의 ContentFormHost → 수명 효과와 Carrier 통지 →
RawCampaignLifetimeBindings/ContainedUnpop → ContainedCreate →
기술/TotalMade/Deck/Production/Sync/FactoryResearch/Upgrade/Input을
완성한다. 이어 생성자1651 이후 Flare/Construction/Damageable/IceTower/
CarrierSteal/BombPostPop/AcidZap/AcidFence·이동/건물의 모듈을 구성한다.
World의 실제 게시1825, 상위 Runtime/Load/Login·프레임·L2/L3는 남았다.

cpppj/보호 파일 수정·디컴파일·게임/브라우저 실행·자연 완주·실제
포획/희생·특정 적 공격/승리·공중3종 전용 관측은 하지 않았다.
실제 미션은 아직 플레이할 수 없다.
