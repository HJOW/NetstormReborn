# 실제 썬 디스크 발사기 타입 자료와 발사 시작 (2026-10-14)

분석 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 최신 AGENTS.md와 두 LEFT를 다시 읽었다. 보호10.78/CD10.72 PE·TYPE·SHP는 읽기만 했고 게임/GUI/OS/DirectDraw를 실행하지 않았다. 이번 자료는 후속 단위11이며 단위08의 합성 회전 입력과 단위10의 Impact/Defense 자료는 보존한다.

## 실제 값

| 항목 | 10.78 | CD10.72 |
| --- | --- | --- |
| 파일/타입 번호/표시명 | sunArcher/71/Sun Disc Thrower | 동일 |
| group/theme | archer0/sun0 | 동일 |
| orientations 속성 | 없음 | 없음 |
| 초기 TYPE+110 방향수 | 0 | 0 |
| TYPE+114 클러스터 수 | 35 | 35 |
| 실제 SHP 두 번째 블록 프레임 수 | 35 | 35 |
| default/baseframe | P32/물리32 | 동일 |
| gumpframe/help | P33/33, A00/34 | 동일 |
| P 글자 Run/A 글자 Run | first0/count34, first34/count1 | 동일 |

타입 파일은 P00~P33과 A00을 이 순서로 정의한다. **35는 회전 방향 수가 아니다.** P32의 기본 그림은 `sarcher.gif #00`이며 실제 Fire 생성자의 현재frame 각도 변환은 `32−16=16`, `16×−22.5+360=0도`다.

보호 TYPE의 경제 값은 두 판본에서 다르다. 패치는 maxHitPoints300/range9/hpPerSec15/cost300이고 CD10.72는400/8/10이며 cost 속성이 없다. 이 파일의 관련부분 AST 관찰은 해당 경제 속성의 전체 callback 복원을 주장하지 않는다.

## 원본 함수와 초기화

| 몸체 | 10.78 | CD10.72 |
| --- | --- | --- |
| 전체 타입 초기화 | `0049ebb0` | `004434d0` |
| CD 단일468바이트 초기화 | 패치 몸체에 인라인 | `00444230` |
| parser stage 진입 | `0049c2e0` | `00446080` |
| 전체 TYPE callback | `0049c3b0` | `004460f0` |
| 후처리/16글자 Run | `0049b0d0`/`0049b060` | `00444e10`/`00444da0` |
| group 이름표 wrapper/순회 | `004a8960`/`004a87c0` | `00420500`/`004204a0` |
| 실제 GunProcess Frame | `0046b830` | `004342f0` |

패치는188×500바이트, CD는171×468바이트를 실제 원본 명령으로 모두 초기화했다. CD의 별도 초기화 함수171회도 실행됐다. +110/+114는 모두0이며 gump/default는0, help/base는−1이다. 테마 초기값은 패치5/CD4이나 실제 theme="sun" callback에서0으로 바뀐다.

초기화 함수는 배열 대입 이후 **실제 parser 진입에서 정지**했다. 정상 반환이나 전체 자산 로딩 완료로 세지 않는다. 관련부분 AST는 보호 TYPE의 실제35클러스터/특수flags, group/theme와 선택적인 orientations 대조 속성만 공급한다. 전체 callback과 후처리는 정상반환까지 실행하며 실제 float 절삭·ASCII 비교·소문자 변환·group 이름표와16글자 Run 몸체를 실행한다.

파일 읽기/힙/현재CRT 스레드는 명시 입력 경계다. SHP는 보호 파일 전체 바이트를 직접 매핑하고, 앞선 dude 처리 뒤에 해당하는 두 번째 블록 커서를 입력으로 제공한다. 앞선 dude 로더나 전체 TYPE lexer/문법parser·경제/소리/효과 속성을 완료로 세지 않는다.

## 회전 생략과 실제 소비자

GunProcess stage2는 `(group!=0 || type==windArcher)`이고 rain/thunder Cannon이 아닐 때만 Rotate를 생성한다. 실제 sunArcher71/group0은 **stage2→4→5→Fire 생성→6**으로 진행한다. 따라서 현재 실물 자료에 방향0을 공급해도 Fire 경로는 회전의 나눗셈을 호출하지 않는다. 직접 Rotate에 방향0을 공급하는 입력은 복원 소비자가 정상 회전 계약으로 받아들이지 않는다.

`ReadRawArcherTypeData`는 보호 Definition의 orientations가 없으면 초기0을 유지하고 클러스터 수를 실제 frameCount로 공급한다. 명시적인 숫자는 float AST 폭으로 변환한 뒤0쪽으로 절삭하며 숫자 이외 또는 정상 int32 범위 밖 입력을 거부한다. 코드에20/40을 실제 기본값으로 주입하지 않는다. 기존08 `RawArcherAnimation` 코드/보고서/1690자료는 불변이다. 그 헤더의 이전 "미공급0" 설명은 당시 직접Rotate 계약의 한계이며 Fire의 실제 방향0 자료를 부정하는 근거로 쓰지 않는다.

`ArcherTypeData_ActualAssetsGunSkipsRotateAndStartsFire` 초안은 같은 보호 GameAssets/현재TypeTable·실제Factory71/158 생성자·같은 SID풀/Host/Kernel/Gun writer와 발사 factory를 연결한다. 현재fort/default 선택 경계의 P32를 제공하고 실제 SHP 크기로 Pop한다. 실제 Gun 세 프레임은 Rotate 없이 Fire를 생성하며 각0을 확인한다. 첫Fire 틱은 실제 중심·건물 표면 통지·Unpop/Repop·SHP 표시 변경을 소비한다. 검사 후 Form을 명시적으로 삭제한다. 전체 발사 종료/실제 Missileimpact·피해·AI·캠페인 월드 조립이나 완주 완료로 세지 않는다.

## 독립 관찰과 현재 상태

`decomp_archertype_oracle.py`는 양판본 **정상42개와 parser접두2개**를 별도 기록한다. 정상마다 callback+후처리 두 top body가 반환하므로 정상 top 호출은84회다. 실제 무orientations/원래 순서2개와 숫자속성·역순·help 제거 대조40개를 구분한다. 각 정상 입력에서 directions/count/특수4인덱스/letterKinds·실제SHPcount와4바이트 코드 전체·16개 first/count 배열을 기록한다. assertion/OS 호출은0이다. smoke와 같은 입력 재생성은 신규 고유 관찰에 더하지 않는다.

최초42/접두2의 전체 source/header/test/oracle/fixture/report6자료는 `cpppj/hist/archertype-initial`에 실제 바이트로 보존했다. 등록 전 숫자AST 거부·직접 SquidPop include와 C++ 오류계약 검사만 추가했으며 `analyze_archertype_draft_lineage.py --verify`가 역변환과 전체 이전/현재SHA·같은fixture를 감사한다. 추가native/고유 입력은0이다.

단위10 **a349e63** 뒤 좁은 `RegularProcess::ResetTask`를 공유파일에 추가했다. 기준커밋의 전체 기존2소스/5보고서/5fixture는 Git LF와 당시 작업원문 바이트를 각각24자료로 보존했고 `analyze_dude_regular_reset_lineage.py`가 기존본문 불변을 감사한다. ArcherType는 이 API 추가 전 전체8자료도 별도 보존하여 같은42/접두2 보고서의 현재공유소스 SHA를 연결한다. 같은50.5좌표의808/555픽셀이 보이는1024×768 viewport 입력을 공급할 때도 직전전체8자료를 보존했다. 현재 draftlineage schema3는 세 단계의 전체source/tool/report/fixture와 현재SHA를 역변환한다. 원본 입력 추가는0이다.

현재 native/lineage SHA 감사는 통과했다. **C++4검사는 공유 Release 실행 전**이다. 등록 대상은 `RawArcherTypeData.cpp`, `ArcherTypeDataTests.cpp`, `NETSTORM_ARCHERTYPE_FIXTURE`이며 감사 명령은 다음과 같다.

```powershell
python -X utf8 tools/decomp_archertype_oracle.py --verify
python -X utf8 tools/analyze_archertype_draft_lineage.py --verify
```

후속은 실제 DudeNoticeAround의 후보·피격 상태·대피 표적/경로와 현재 월드 소비자를 별도 원본 몸체로 복원하는 작업이다. 단위10의 Ground 경계와 전체 캠페인1-1 동일성은 계속 미완료다.
