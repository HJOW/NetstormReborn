# Session의 너겟·내용물 미리보기와 운반 주기 (WB04-E608)

`225553df` 생성자1659..1664와 ConnectCarrierRegular2462..2564를
같은 World/Core/시계/표면에 연결했다. 대상 Session.cpp와 NuggetPostPop/
Regular·CarrierRegular/Falling/Task/FallStart·ContentPreviewProcess의
기준선 뒤 변경은0이다. RawCarrierTask의 종전1f00a25a 이후 소스도 같아
대응표 base를225553df로 확인했다. Native 장기 몸체 대여는 Client 결선에
남았다. 독립 typed 분기에 별도 근사 월드를 만들지 않았다.

## 연결한 수명과 정밀도

- 다섯 World runtime 훅은 실제 이동 TYPE·contained 경로 조회·현재
  회피 서비스·낙하·소유권에 연결했다. 회피 함수 표는 입력 값을 복사하고
  호출 시 현재 Session 서비스를 읽으며 반환값을 버린다.
- 너겟 PostPop/왕복 끝점은 같은 실제 미부착 Regular 한 개와 게임 난수를
  사용한다. LCG상위16비트/65536과 float 구간 끝·마지막 float 저장,
  정수 난수의 상한 제외를 유지한다. Attach 성공 뒤만 현재 시각/count0으로
  ResetTask하며 NULL 부착은 원문처럼 별도 예외를 만들지 않는다.
- 같은188 프레임 표의 A구간 수와 실제116 정의의 기본 그림/casttime을
  같은 번호로 보충한다. casttime은 기본0·유한/signed32 범위 검사 뒤
  0방향으로 버린다. 오류 때 앞서 만든 너겟 몸체를 보존한다.
- 내용물 타이머는 같은 실제24BYTE 필드/확보 객체를 현재 시각·현재
  Sound Process TYPE로 생성해 실제 Host에 붙인다. 필드 동일성·중복
  확보를 검사하고, 부착 앞에 예약 소유권을 소비한다. 정확한 만료에는
  부모의 가상 삭제가 같은 타이머/Kernel 등록까지 반납한다.
- 낙하는 Hash0의 살아 있는 `view(0)`와 같은 Player/현재 TYPE·프레임을
  사용한다(`entries(0)`은 사본이므로 사용하지 않음). checking 사본은
  같은 전역 owner를 계속 읽는다. 첫 kind 조회는 mask 없이 같은 실제
  Form6 체인을 순회한다. UI는 현재 약한 수명 토큰만 사용하며 미설치는
  선택된 위치에서 명시 오류다. 실제 Selection 생성은 후속 외곽 단계다.
- Carrier task의 pending은 크랩과 공유한다. payload5·Regular/SharedRegular
  선택·기존 낙하 유지·현재 기한을 보존하며 새 부착 후 event/count를
  초기화하지 않는다. 반납 때 상위가 Host를 먼저 비운다.

## 최초 실패와 검증

신규 최초13건 중12건이 통과했다. 실제 task에0.2를 넘겼을 때
`11.45`가 나와 원문의 `11.25 + float(0.2)`와 달랐다. 기존 RawCarrierTask의
ScheduleTime이 유한 delay를 좁히지 않고 더하던 결함이었다. 유한 경로에
`Math.fround(delay)`를 적용했고 NaN의 payload/부호 분기는 유지했다.
추가 만료 검사에서는 Host 포인터가 null로 지워질 것이라는 검사측 가정을
제거하고 실제 계약인 Kernel 등록 반납/필드 소멸 표식을 검사했다. 제품을
우회하지 않았다. 최초 명령의 Python 보조 경로는 cwd가 webpj여서 실패해
루트 기준으로 다시 실행했다(검사 로그 자체는 정상 생성됨).

신규16건과 기존을 합친 관련41건을 통과했다. 기존 원본 관측 파일은 다음과 같고,
새 Source 결선 검사를 원본 관측으로 등록하지 않았다(manifest806 유지).

| 기존 관측 파일 | 행 | SHA-256 |
| --- | --- | --- |
| `nugget-postpop-x86.tsv` | 288 | `46998ea686990586fb51f7a50f6cade479ec339bfa8187e99b9e6cff4367fa7a` |
| `nugget-regular-x86.tsv` | 9019 | `185f7df7e9600e2e1f1d7d622b0a7de3c4382bb5f1347309822326d5bf50f1e6` |
| `carrier-fall-start-x86.tsv` | 2850 | `ed68324eaa72914ed304434e148bfb563dcebf24f11d04c17f93599d5501b686` |
| `carrier-falling-x86.tsv` | 9884 | `15f5eebe0a06307036e1e863a0c516e12d919f0303da8d0ab8ee3753db774367` |
| `carrier-regular-x86.tsv` | 837 | `90b9e5fbe46a542bd074467ef2be2da38a0482a6faf63aab0976c9dc35c7b5dd` |
| `content-preview-process-x86.tsv` | 957 | `c05105f5e39cb0ce50682719b5494e65d67cb64f1db08c29882a89ef9837f948` |

CarrierRegular VT83행·CarrierFalling VT8행도 기존 검사에서 대조한다.
Node v22.23.3·v26.13.0 전체는 각각5,861건 통과했고 실패·취소·건너뜀은0이다.
두 실행의 exit0과 최종 요약을 확인했다. 대응표 오류0, UTF-8 7파일·diff 검사도
통과했다. JS131,886줄/시험778파일이며 이식 수치는846/39/122·session0을
유지한다. Node24는 사용자 지시로 생략했다.

실제 Session의 CarrierPickupNugget/CarrierHover 및 UI·경로·수신 등록은
Load/World 전체 분배와 입력 조립 뒤 재대조한다. 특히 CapturablePriest의
실제 포획/희생 실행은 기존 사용자 경계를 따른다. World 분배1770/게시1825,
외곽 Session/Load·Login·프레임·L2/L3와 실제 미션 플레이는 남았다.
다음은 생성자1665..1667의 Sun/Wind/RainAviary다. 공중3종의 새 전용
생산/생성/행동 관측은 만들지 않고 해당 구간은 정적 결선을 우선한다.

보호 원본/cpppj 수정·디컴파일·게임/브라우저 실행·실제 사제 포획/희생·
특정 적 공격·승리 재현을 하지 않았다. 로그는 extracted/wb-e608- 접두의
initial·focused·related·related-final·start-port-map·port-map·node22·node26이다.
