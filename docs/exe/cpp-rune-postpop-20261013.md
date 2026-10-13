# Rune Pop과 건설 후 최초 예약 — 2026-10-13

마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-13**이다. `runepostpop` **패치3/CD3**를 같은 정밀 프로젝트에서 readOnly 내보냈다. 일반 finder/좌표 helper는 앞 `templeritual`/`pathcleanup`의 현재 출처를 읽기만 재사용한다. 보호 PE/AGENTS/이전 완료 근거는 보존한다.

## 실제 순서

| 역할 | originals 10.78 | originalCD 10.72 | C++ |
| --- | --- | --- | --- |
| Rune166 postPop | `00448930` | `00484520` | `RawRunePostPop::PostPop` |
| 근처 같은 owner dais | `00448a20` | postPop에 인라인 | `FindOwnedDais` |
| altar/player 건설 시간 | `00448340` | `00482990` | `ConstructionSeconds` |
| 기준점의 altar | typed finder 인라인 | `004816a0` | 기존 실제 일반 finder |
| local304 Regular | `00496e80` | `0048efd0` | 같은 `Host.AddRegular` |
| 건설 시작 | `00443fe0` | `004d06a0` | 기존 `RawConstructionProcess::Start` |
| 공통 postPop | `004b0d30` | `004ae180` | 같은 `SquidPostPop::PostPopBase` |

flags1이면 new0x28 성공 시 **Rune/event0x304/payload0.01F의 local Regular**를 만든다. SharedRegular로 바꾸지 않는다. 확보 실패에도 후속 처리는 계속한다.

flags2이면 현재 Rune 위치를 원본 bias **0.9999899864196777F**로 거의 올리고 반경1의 일반 finder를 순회해 현재 type165/같은 owner의 첫 신전을 읽는다. dead/abstract/free를 별도 제외 조건으로 추가하지 않으며 generic finder는 buried 후보를 제외한다. 찾으면 Rune을 부모로 기존 건설 시작을 부른다.

시간 helper는 전달한 객체 기준점의 첫 altar164를 typed point finder로 조회한다. **PostPop 내부에서는 발견한 Dais가 이 helper의 this**다. 양판본 원본 ECX를 제한 PE에서 추적해 독립 시간 조회는 Rune8, PostPop 내부 시간 조회는 Dais6임을 확인했다. 따라서 Rune과 Dais의 위치가 다르면 독립 Rune 시간과 실제 건설 요청 시간이 다를 수 있다. altar가 없거나 원래owner WORD가0이거나 억제 전역이 켜져 있으면10초다. 나머지는 원래owner WORD의 playerFlags bit1이0이면7초,1이면25초다. 패치는 player+0x50의 **BYTE**, CD는 **DWORD**를 검사한다. CD 억제 분기의 추가 로컬 owner 비교 이후에도 같은 현재 전역이 켜져 있으면10을 반환하는 원본 순서를 기록했다. 정상 인수인계/대조는 owner0/1/8이며 캠페인 밖 challenge player 범위 확대는 별도다.

마지막에는 예약/신전 존재와 무관하게 같은 flags의 공통PostPopBase를 한 번 실행한다. 새 모듈은 기존 장부·Graph·깊이 몸체를 복제하지 않는다. `MakeRunePostPopHooks`는 같은 풀의 실제 Host/Construction을 연결하며 예약 확보 실패는 caller가 공급한다.

## 기존 공통 후처리 보존

SquidPostPop의 새 API는 `SetRunePostPop`/`HandlesRune`이며 실제 타입166와 PE 가상표 **005075d8/CD00504148** 및 명시 등록이 모두 있어야 일반 SquidPop이 Rune을 받는다. vtable을 base 주소로 덮거나 기존 전체 지원표에 미연결 Rune을 추가하지 않는다. Validate와 Activate에 Rune 분기만 추가하고 이 몸체가 내부에서 공통 depth를 한 번 감소시키는지 확인한다.

`tools/analyze_runepostpop_lineage.py`는 기준 **b76adf9**에서 새 선언/정의/등록/검사/분배/설명만 역제거하면 기존 .h/.cpp의 전체 문자/토큰과 같음을 증명한다. 기존 다리/섬/사제/보호막/Carrier/Factory/자산의 본문 전체를 비교한다. 이전 공통/Pop/사제/건설 보고서와 PE/export/oracle/fixture는 그대로다. 등록 추가의 신규 원본 관찰은0개이며 앞 건설2868개를 다시 합산하지 않는다.

## 독립 원본과 실제 소비자 준비

최종 native는 **정상1920개/assert0개**, 판본별960개이며 각 x87 정밀도에서 같은 결과로 반환했다. 각 판본의 독립 시간 결과는 **10초704 / 7초128 / 25초128**이다. 신규 관찰은 원본 Rune postPop/같은 owner 조회/시간 helper이며 new/Regular/ConstructionStart/base는 명시 요청 경계다. 그 기존 body 관찰을 새 Rune 입력에 추가 합산하지 않는다.

최초960개 정상 입력에서는 순환 입력의 suppress와playerFlag가 서로 연관되어25초를 검사하지 못했다. 최초 fixture/report/oracle는 `extracted/runepostpop-before-duration-coverage-*`로 보존했고 억제/플래그를 독립 교차해1920개로 확장했다. 최초960과smoke20은 최종1920에 더하지 않는다.

첫 C++ 검사는 실제 조회/시간/요청 순서와 후보6/7/Rune8 raw 전체(vtable 주입 DWORD 제외), 확보/Regular callback 뒤 현재 전역·owner를 대조한다. 공통 장부의 실제 depth0도 확인한다.

08 첫 공유 대조는 실제 Pop/Construction 소비자에 성공했고 전수 대조의26개 check가 실패했다. 모두 Dais가 두 칸 떨어진 입력에서 시간 helper에 Rune을 전달한 C++ 오류였다. `ConstructionSeconds(rune)` 한곳을 `ConstructionSeconds(dais)`로 교정했다. 원본 fixture1920개는 전체 바이트가 그대로이며 같은1920개를 전량 재관찰해 현재 SourceSHA를 연결한다. 수정 전 production/test/tool/fixture/report는 `cpppj/recovery-history/temple-rune-initial`에 원경로로 보존했다. `analyze_temple_rune_runtime8_lineage.py`는 이 인수 교정/설명 한줄 외 전체 소스·검사·등록 근거의 불변을 확인한다. 새 고유 입력은0개이며 이전1920개를 더하지 않는다. 임시 진단 출력은 제거했다.

두 번째 검사는 실제 Rune 생성자/owner/공통HP0→abstract **SquidPop**→같은 Host의 실제 ConstructionProcess(7초)→quickBuild 진행/HP/실제표시→실제 Unpop→abstract 해제→실제 Repop(flags9)→같은 Host의 **304 localRegular**, 비용/통계/공통 depth0을 실행한다. 초기 dais/altar hash는 명시 합성 입력이다. 진행음 확보 실패와 readySound0을 공급하며 실제 contained finder로 소리 프로세스 부재를 읽는다. 위치 음향/건설 시각 효과는 요청과 횟수로 관찰한다. Rune304 사건 자체는 실행하지 않는다. C++ 공유 빌드/집중 결과는 root가 통합 후 기록한다.

교정 후 공유 Release빌드71712가 종료0이며 **RunePostPop의2검사,1920개 전수+실제 건설/304 예약 소비자 모두 실패0**이다. 집중 로그는 `extracted/campaign-runtime8-rawRunePostPop_final-tests.log`이다. 이 성공은304의 예약까지이며 사건 몸체 실행/전체 월드·캠페인 완주를 포함하지 않는다.

```text
python -X utf8 tools/decomp_runepostpop_oracle.py --verify
python -X utf8 tools/analyze_runepostpop_lineage.py --verify
python -X utf8 tools/analyze_squidhitpoints_lineage.py --verify
python -X utf8 tools/analyze_temple_rune_runtime8_lineage.py --verify
```

## 남은 범위

Rune304는 같은 owner Dais의 실제 Progress를 호출한 뒤 Rune을 flags0x48로 삭제하고-1을 반환한다. 이 사건과 신전305/306/307 Regular 분배, 실제 룬/신전 player/프레임 전역, 위치 음향·건설 시각/소리 프로세스, 사제 운반·채굴·경로 명령/도착의 실제 GameWorld 연결과 UI·캠페인 완주 동일성은 후속이다. 위의 실제 Pop/Construction 소비자 검사를 통과하기 전에는 Rune 월드 동작을 완료로 표기하지 않는다.
