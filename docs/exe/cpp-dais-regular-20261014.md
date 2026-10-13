# Rune/Dais 의식 사건 사슬 복원 진행 — 2026-10-14

마지막 디컴파일 PC는 **HJOW-Athlon,2026-10-14**다. Dais 현재 순차 Refined readOnly 내보내기는 **패치18/CD10.72 22함수**, 종료0이며 이전18/21 몸체는 `extracted/daisregular-before-cd-stop-wrapper`에 보존했다. CD Progress의 강한 사제→604 취소 wrapper`00482950`을 추가했다. 원본/AGENTS/dotnetpj와 이전 완료 보고서는 보존한다.

## 새 전용 상위 몸체

`RawDaisRegular` 초안은 Rune304→Dais Progress→Rune flags0x48 삭제, Dais305 단계 경계/306 전역169 희생 구간/307 의식 후속을 분배한다. 기존 TempleRitual의 count/Stage/좌표/finder, Rune의 same-owner Dais/AltarAt, Carrier 선택, Host/Frame/PathStart를 재사용한다. O 계층은 client 헤더를 포함하지 않는다.

| 역할 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| Rune304 Handle |0044a3a0|004846b0|
| Dais305/306/307 Handle |0044b150|00482c40|
| Progress |00449f40|00482a20|
| 사제 K..O→604 한개 삭제→E |00491b60|0040c840/0040c800|
| collapse |00448080|00482370|
| 희생 시작/구간 진행 |004483c0/0044a200|00482cf0/00482df0|
| 희생 종료 |00449b30|00482620|
| 다음 목표 자격/명령/깊이 |00449710/00449900/00427d30|00481910/00481ab0/004e7660|
| 획득 타입/altar 이탈 |00448260/00448c80|00482560/00483cd0|

305는 explicit 다음 프레임의 Stage도 현재 raw count로 판정한 뒤 항상 다음 프레임을 지정한다. 같은 단계면0.1, 경계면0을 돌려준다.306은 현재 Dais 타입의 generic Advance가 아닌 **전역169 코드표**를 사용한다. 다음 프레임이 현재보다 작으면 extra2를 끄고 FinishSacrifice한다. 정상 지연은0.12F다.

Progress의 최종 단계는 두 지연 소리를 독립 확보한다. 포획 Dais가 있으면 `priestSacrifice4.wav`/1.5초, 없으면 `priestSacrifice2.wav`/4초다. `thunderCrack.wav`는 실제 double2.799999952316284초다. 시각은 부착 성공 뒤의 벽시계이며 shared Regular의 게임 시각과 구분한다. 아직 실제 client SoundProcess 어댑터/장치 소비자는 작성 전이다.

획득 타입의 패치 비로컬 SP/다른 타입 전달을 CD에 추가하지 않는다. CD localSP5000은 Get/Set/표시이며 기존 Reward.AddSp의 AI 지갑 효과를 넣지 않는다. 희생 업그레이드 다음 첫 프레임의 실제 PE 데이터는 count1→64/count2→120/count3→0이다.

## 현재 독립 관찰 범위

`decomp_daisregular_oracle.py`의 첫 범위는 **Stop604/Collapse/StartSacrifice/169의 비wrap Advance/305/CanBridge**, 정상668개/assert0개다. 두 x87 정밀도에서 같은 출력으로 반환하며 `daisregular-core-x86.tsv`와 현재 source SHA를 보존한다. 최초 smoke40개와 재실행은668에 더하지 않는다. 첫 smoke의 CD altar helper 누락은 기존 Rune 내보내기 몸체를 읽기만 재사용해 해결했고, 첫 전수의 Kill ABI는 thiscall/인수0/ret4로 교정했다. 최초 로그는 보존한다.

프레임 setter/new/Shared/위치 음향/Host FindEvent·Kill은 명시 요청 경계다. 단위11 첫집중 C++ 검사에서 **668전수1검사/실패0**을 확인했다(`run11-rawDaisRegular_first.log`). 그 Stop604는 같은실제Host/Kernel 삭제를 소비한다. 전체 Progress·Bridge/Rune304의 원본대조·실제소비자는 미수행이다. Altar후속의2866은별도독립몸체이며기존668에겹쳐합산하지않는다. 새범위의게임 의식완주로표시하지않는다.

단위11 최종 Release도 경고0/오류0이며 다이스1·알터1을 포함한 새15검사와 전체1038검사/실패0을 확인했다. 현재668 자료·생산/검사 전체 SHA는 최종 종합 감사와 일치한다. 후속 단위의 자료는 이 완료 입력수에 합산하지 않는다.

## 공통 helper 보존

AltarAt/MoveAwayDepth 공개는 기준08`552656c` 전체 기존 문자/토큰과 별도 `dais-shared-lineage.json`으로 연결하고09`5b10ce8` 준비 부분에 포함했다. 기존 Rune2/Runtime4/Process3/Start2/Cleanup2의13 회귀 검사는 성공했으며 이전 native 수치를 새 Dais에 합산하지 않는다.

Carrier의 선택인수`allowDead=false`와 Begin 전달만 추가했다. Dais wrapper`00448170/CD00482430`은true,CanBridge는false다. `analyze_dais_contained_lineage.py`는 기준09`5b10ce8`의 전체 h/cpp 문자/토큰과 기존 Carrier/contained/09 원문·출처 보존을 증명한다. 이 API 준비의 추가 고유 입력은0이다. 기존 shared/Carrier 파일은 다음 공유 빌드 전 현재 상태로 동결한다.

## 일반 포획 사제의 필수 후속

stage4에서 포획 Dais가 없는 일반 사제는 **AltarEscape→altar event24**로 진행한다. altar164의 실제 Handle`0044a5e0/CD00483800`은24에서 Dais 이탈 프레임 helper`004486d0/CD00483040`, FrameAdvance, 포획 사제 삭제/폭발과 Dais 가상삭제flags0x400000을 실행한다. Dais 실제 preDestroy`0044afd0/CD004821e0`은 altar의 현재24 Regular를 찾아 FinishSacrifice를 호출한다. 따라서 Dais306만으로 일반 희생 완료를 주장하지 않는다.

현재 별도 `altarsacrifice-functions.json`은 실제중심helper를포함한 **패치9/CD8**이며같은PC/날짜에서읽기전용내보내기종료0이다. altar20/21/22/23/24, Dais 이탈/해방 및 preDestroy·원본Finish 사슬의독립2866과C++1검사/실패0을별도기록했다. 처음8/7·30smoke/중단전수와초기Frame입력교정전원문은 `hist`의두전용경로에보존한다. CD 해방은 현재 로컬 owner일 때`enemyPriestEsacping.wav`도 추가 재생하고 패치 altar21의 동맹에 따른 자동 해방 분기는 CD에 없다. 실제 전체DamageablePostDestroy/파생Pop등록/player/UI/삭제/음향/호스트·GameWorld 연결과 캠페인1-1 의식 완주는 후속이다.
