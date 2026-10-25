# 실제 Player 신전 SID에 따른 사제 HP 회복 (단위447)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 디컴파일 날짜는 **2026-10-23**이다. 기준 HEAD는 `752d5713`이다. 이번에는 기존 정밀 C와 보호 PE를 읽기 전용으로 사용하며 디컴파일 산출물·기존 fixture·원본 게임 파일은 변경하지 않는다.

## 문제와 연결

원문 HP setter `004942b0/CD0040ca40`은 낮은 HP가 최대 HP의 절반 이상으로 전환할 때 현재 Player의 신전 SID DWORD가 0이면 이전 HP로 되돌린다. 기존 `PriestHitPointMode::recoveryAllowed`는 독립 fixture 입력용 배열이었고 실제 Session에서는 기본 false로 남아 있었다. 신전이 있는 Player의 사제도 이 경계를 넘어 회복할 수 없는 상태였다.

이제 실제 Player9의 읽기 span을 HP 모드가 빌린다. `RecoveryAllowed(owner,edition)`는 PATCH `stride0xb4/+0x7c`, CD `stride0xac/+0x74`의 현재 네 바이트를 읽는다. SID의 하위 WORD만 읽거나 객체 유효성·free/dead 필터를 추가하지 않는다. 비권한 전환·전환 없음·HP가 낮아지는 전환은 원문 분기대로 이 필드를 조회하지 않는다. Unpop 전에 허용을 결정하므로 Unpop이 신전 필드를 0으로 바꿔도 현재 Repop은 수행하고 다음 회복부터 0을 반영한다.

`RawPriestWorldBindings::BindPlayerRecords`가 판본별 전체 폭과 같은 주소를 검증한다. 실패한 빈 입력·다른 폭·다른 주소는 기존 대여를 덮지 않는다. 실제 PATCH Session은 Player 초기화 뒤, 첫 사제 로딩 전에 기존 `ClientPlayerRecords::Use`의 고정 저장소를 연결한다. 이 대여는 사제·회복 처리기보다 나중에 반납되므로 mode 복사와 Regular 호출도 같은 현재 필드를 읽는다. CD와 추가 10.37은 기존 독립 월드·PE 비교 경로이며 CD 제품 Session 완성을 뜻하지 않는다.

## 원본 근거와 신규 검사

- 두 정밀 C의 HP setter와 SHA를 `extracted/cpp-priest-recovery-447-static/`에 보관했다. 전체 정밀 디컴파일은 다시 실행하지 않았다.
- 기존 Native setter **3,936행**(판본당 1,312행)을 실제 Player span과 원본 입력에 반대인 값형 bool 배열로 재생한다. 기대 HP·전체 raw·공간 호출 순서는 기존 PE fixture 그대로다.
- owner0·1·8, DWORD 상위 비트만 켜진 값, mode 복사, 불필요한 조회 생략, 잘못된 기록 폭, Unpop 뒤 현재 필드, 실제 Regular·보호막 정리, 실제 Client/Session을 검사하는 신규 **7사례**를 추가했다.
- 원본 세 PE를 격리한 x86 에뮬레이터에서 **63입력·실제 setter126회**(판본당42회), 두 x87 정밀도와 가상 Unpop/Repop 각24쌍을 관측했다. 신전 DWORD가 0인 경우, `0x10000`·`0x80000000`인 경우, Unpop 경계가 필드를 지우는 경우를 포함한다. 원본 게임 창과 OS 코드는 실행하지 않았고, 이 관측의 Unpop/Repop은 명시한 외부 경계다.
- 변경한 검사3파일의 기존 **231사례 바이트**와 관련 fixture4개의 내용을 보존했다. fixture의 Git LF/작업 트리 CRLF는 체크아웃 개행으로 구별하고 양쪽 SHA를 기록했다.

## 검증 결과

Source1의 **2,027소스 동결**, build1 종료0·경고/오류0, 집중 **479고유/480관측·실패0**(신규7개 전부)을 확인했다. 같은 최종 게임의 전체 CTest **3,052개·실패0**, 메뉴 GUI2경로, 자연 Early2 A~I 안내·회수75·SP3075·Tutorial3 진입을 통과했다. 이전3,027개와 445·446의 추가18개, 이번7개의 실제 이름 집합이 현재 소스와 모두 일치한다. 보호1,821파일·원본/CD 전체 파일 집합·허용 설정 복원도 전후 일치했다.

게임 SHA는 `97bef8590b8711bbe7e23bdbd03166adca159103e00c53c31e57cf2fbf1debb5`다. [실행 보고서](../../cpppj/priest-recovery-runtime-20261026.json), 원시 `extracted/cpp-priest-recovery-447-*`에 빌드·집중·Native·통합·화면 확인을 보존했다. 이 화면 확인은 원본과의 픽셀 동일성 검증이 아니다.

## 남은 목표

Native State1 전체 cleanup/prefix·반납 coordinator와 Edit 제품 진입, 기존 SID16 변환·전체 Deck 명령 부모, Early3~6·전체 캠페인·Priest Training·데모 반복의 자연 진행, 표시·음향 동일성은 별도 작업으로 남아 있다. 이 회복 연결과 좁은 검사를 전체 게임 완료로 판정하지 않는다.
