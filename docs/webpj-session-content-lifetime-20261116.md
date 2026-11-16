# Session의 실제 ContentForm 수명·운반 통지·생성 (WB04-E597)

`225553df`의 `RawCampaignSession.cpp`3050..3134 독립 typed 분기를
같은 owner에 연결했다. E596의 통지 접두 뒤부터 ContentFormHost·수명·
운반 통지·종속 해제·ContainedCreate까지다. 해당 cpp와 사용한 기존
몸체/SoundProcess의 기준선 뒤 변경은0이다. 생산은3135부터 남았다.
Native 장기 Storage 분기는 실제 Client 결선 때 이어간다.

## 같은 Pool·실제 수명과 원문 순서

- 현재 ContentForm 상태와 같은 Pool/TYPE로 `ContentFormHost`를 만들고
  현재 World TYPE 표를 빌린다. 일반 부모 통지/포획 안내는 E596의 실제
  객체로 이어진다. 이 구성만으로 통지나 포획 동작을 실행하지 않는다.
- 수명 효과는 서비스 값 사본이며 커서 취소는 후설치 현재 cursor를
  선택한다. 커서 DWORD는 같은 cursorDrop.preview, 로컬 소유자는 설치
  순간 battleOwner의 BYTE 값, 수입 사건은 원문0x020000b4다. 빈 장치는
  실제 FillMissing 위치에서만 실패한다. 연결/NULL 원격은 자연 생략하며
  연결된 원격5a 송신의 기존 미복원 오류를 그대로 유지했다.
- Carrier 중심·선택 내용물 Regular·현재 TYPE activeSound/pickupSound
  조회·번개/전역·위치 소리·현재 World 타입 탐색·후설치 DamageablePost
  genus 탐색을 실제 부모에 연결했다. 6c/7c 외 TYPE 조회 함수만 기존
  서비스 사본을 보존한다. 나머지 사용자 제공 효과도 원문처럼 우선한다.
- Sound47의 new40 요청은 **실제 미부착 SoundProcess 하나**를 확보한다.
  JS에는 미생성 객체 저장소의 placement new가 없으므로, 효과가 없는
  초기 필드만 가진 객체에 create 단계에서 생성 인자를 채운다. 새
  initializeUnattached는 이미 Host/PID/Form이 있는 객체를 거부한다.
  같은 예약 객체를 한 번만 실제 Host에 현재 Sound TYPE/flags10으로
  넘긴다. 별도 토큰용 바이트나 새 객체로 예약 성공을 가장하지 않았다.
  부착 실패는 기존 Host 반환을 따르며, duration은 현재 Form의 실제
  SoundProcess를 조회해 단정도 지연+벽시각을 저장한다. 미부착 예약은
  부모 반납 때 해제한다. JS VM의 복구 불가능한 OOM을 모사하지 않는다.
- Carrier 상태는 현재 저장/로딩/폭탄 관문·소유자·사제/레인 워커와
  **전역 camera snapshot**의 픽셀을 조회한다. Renderer의 별도 현재 뷰를
  대신 쓰지 않는다. 부모 분배는 현재 다이스→현재 연구→현재 Carrier→
  서비스 사본 순서이며, 실제 Player는 LifetimeBindings의 기존 몸체다.
- 같은 World/hp/Player 통지로 `RawCampaignLifetimeBindings`를 만들어
  Form6 수명을 등록한다. 현재 VT별 부모/이전 링크를 사용하는
  `RawSquidContainedUnpop`을 같은 World에 대여한다. UpdateContents 큐와
  진단 함수는 원문 캡처 시점의 사본이고 인자는 signed32 비트다.
- Content의 Pop/Unpop은 같은 Factory를 사용한다. 생성·Regular 뒤
  재부착은 현재 VT6이면 정렬 부착, VT1/2/3이면 공통 부착을 선택한다.
  다른 VT는 미복원 오류다. 마지막 `RawContainedCreate`는 같은 현재
  contained/bridge TYPE와 실제 건설 표면 통지·가상 부착을 빌린다.
- 통지 뒤 한 번만 실행하며 정상 복귀는 lifetimeReady로 구분한다.
  이는 InstallContents 전체 완료나 World/Session 게시를 뜻하지 않는다.

## 근거·최초 실패·검증

새 전용 원본 관측은 없다. 보호 자산/실제 모듈·명시 소비자 계약으로
연결을 확인했다. 실제 생성/정렬/Pop/Unpop·Regular/Sound47 부착은
기존 몸체를 실행한다. 후설치 포인터 선택 검사의 함수 대역은 인자와
분배 순서만 관측하며 실제 포획/해방 장면의 증거가 아니다.

신규16건 첫 실행은15건 통과/1건 실패였다. contained TYPE을3으로
바꾼 검사가 원문의 `getType()!=contentFormType` 진단을 빠뜨렸으므로,
현재 진단 기록 장치를 공급하고 해당 표현식도 단언했다. 제품의 진단을
완화하지 않았다. 최종 신규16건과 E596/수명/Form/종속/Carrier/Sound
**관련86건 통과**다. 기존 SoundProcess의 10.78 **6127행** 관측도
변경 없이 대조했다(`soundprocess-x86.tsv`, manifest의 SHA-256
`1687245d…203`, 전체18386줄; CD/10.37은 웹 대상 밖).

Node v22.23.3·v26.13.0 최종 전체는 각각 **5730건 통과, 실패·취소·건너뜀0**이다. Node24는 사용자 지시로
생략하며22/26 통과 시 호환을 가정한다. 대응표1160행/오류0도 확인했다. 직접 연결된 `sound-process`
manifest1항목을 갱신했고, 도구가 이전 worktree 표시도 실제 커밋으로
확정했다. 총806항목·JS130813행·검사767개다. 대응표 상태와
port845/partial37/todo125·잔여162/34373 C++행·4-D36/51·session0은 같다.

로그: `extracted/wb-e597-initial.log`, `wb-e597-focused.log`,
`wb-e597-manifest.log`, `wb-e597-port-map.log`, `wb-e597-node22.log`,
`wb-e597-node26.log`.

## 다음 시작점

InstallContents3135의 PlayerTechnology → TotalMade/Deck/Production/
Sync/FactoryResearch/Upgrade/Input을 연결한다. 이어 생성자1651의
Flare/Construction/Damageable/IceTower/CarrierSteal/BombPostPop/
AcidZap/AcidFence·이동/건물로 간다. World.BindRuntime1825·상위
Runtime 공개·Load/Login·프레임·L2/L3는 여전히 남았다.

cpppj/보호 파일 수정·디컴파일·게임/브라우저 실행·자연 완주·실제 포획/
희생·특정 적 공격/승리·공중3종 전용 관측은 하지 않았다.
실제 미션은 아직 플레이할 수 없다.
