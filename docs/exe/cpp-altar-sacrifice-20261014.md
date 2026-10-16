# 제단 희생·해방과 다이스 삭제 전처리 복원 — 2026-10-14

마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 현재 `altarsacrifice`는 패치10.78 **9함수**/CD10.72 **8함수**를 Refined readOnly로 내보냈고 종료0이다. `daislifecycle`의 실제 최초Pop/삭제 전후 몸체5/5도 같은PC에서 내보냈다. 보호원본과 앞 완료단위의 근거는 보존한다.

## 실제 상위 몸체

`RawAltarSacrifice`는 제단164의20~24 사건, 다이스165의 이탈 프레임/사제 해방 및 파생 삭제 전처리를 복원한다. 프레임 진행은 기존 `RawFrameAdvance`, 기준점 제단 조회는 `RawRunePostPop::AltarAt`, 포획 사제 kind WORD/순번0/삭제false 조회는 기존 Carrier의 mask0/allowDeadfalse를 재사용한다. 이 정상 specialized 호출에서kind0은 원본과 Carrier의 wildcard 의미가 다르므로0을 거부한다.

| 몸체 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 제단20~24 Handle |0044a5e0|00483800|
| 이탈 프레임 |004486d0|00483040|
| 기준점 다이스 |00448bb0|00483c50|
| 자동 해방 제단 가상공간 접두 |00448bd0|해당21 분기 없음|
| 포획 사제 해방 |00448500|00482ec0|
| 다이스 파생 preDestroy |0044afd0|004821e0|
| 제단 가상 개수 |0044a480|00483670|
| specialized 포획 조회 |004b26f0|004eb260|
| 중심 helper |004adea0|004aef30|

event21은 현재 저항 확률40/주기3초 전역을 사용한다. 권한·포획 사제·동맹에 따른 자동 해방은 패치만 수행한다. 사슬 소리5개와 사제 저항 소리3개 선택은 같은 공유 난수를 소비한다. event20/22는 제단O 구간을 앞/뒤로 진행하고 감김일 때0, 아니면 입력payload를 반환한다.23은 그대로 반환한다.

event24의 다이스K+권한 경로는 실제 가상삭제flags`0x400000`이다. 그 다이스의 preDestroy는 제단의 실제 process Form 체인을 순회하고 현재 Regular/SharedRegular24 **각각**에서 기존 `FinishSacrifice`를 실행한다. 같은 제단의 복수24도 한 번으로 줄이지 않는다. 다음 링크는 현재 후보를 처리하기 전에 저장한다. 이후 강한 사제604취소, 현재 발자국 내 제단flags`0x48` 삭제, owner1..8 갱신과 기존 전체 DamageablePreDestroy를 호출한다.

다이스가K가 아닌24의 감김 경로는 포획 사제flags8 삭제, 애니메이션 제거/폭발 소리, 다이스K/제단M 지정 뒤 **3.0F**를 반환한다. 저항 위치 소리의 중심 보정도 두 좌표에서 **3.0F**를 뺀다. PE DWORD`40400000`을 직접 확인했으며 추측한0.5를 사용하지 않는다. 해방 사제의 Pop 보정은 별도상수`3f000000`의 **0.5F**다.

사제 해방은 현재 priestType flags0 생성→포획WORD복사→가상owner0→두 번 현재 다이스 중심 조회→x+.5 Pop→5.0/0 효과→포획객체flags0 삭제→priestFree 전역소리→604취소 순서다. CD는 현재 다이스owner가 로컬이면 `enemyPriestEsacping.wav`도 재생한다. 해방 중 실제 전체 사제Pop를 공통Pop로 대체하지 않는다.

## 독립 원본 관찰과 현재 검사 범위

현재 `altarsacrifice-x86.tsv`는 **2866정상/assert0** 입력이다. 패치1435/CD1431이며 CD에없는 직접제단자동해방4개를 제외한다. 각입력을 두 x87 정밀도에서 같은 출력으로 반환하고 스택/SEH/FPCW/TOP 복구를 검사했다. 원본 FinishSacrifice는144회/판본 실행됐다. `decomp_altarsacrifice_oracle.py --verify`는 현재 production/test/import/PE/export/fixture의 전체 SHA와 중복 없는 입력키·반환수를 확인한다.

프레임 setter/new/가상공간·삭제/음향/UI/FromForm·Type/마지막 Damageable는 명시 효과 경계다. 원본 FrameAdvance·범위 난수·typed/contained/process walker·Altar/DaisPre/Finish 판단은 실행했다. CD의 SP Get/Set은 실제 원본 몸체를 실행하고 마지막 표시만 격리했다. O 계층이 client 헤더를 포함하지 않는다.

`AltarSacrificeTests`는 같은 실제 풀과 Host/Kernel의24를 만들고 기존 FrameAdvance→SquidFrame을 사용한다. Native의FromForm 관찰Q는 실제 Host 조회·Regular 이벤트/완료 횟수와 별도로 대조한다. preDestroy 입력은 Native의 마지막 Damageable 호출 진입에서 raw/효과/player를 관찰하고 전용 예외로 중단한다. **그 검사로 전체 파괴가 완료됐다고 주장하지 않는다.** root의 단위11 첫집중검사에서2866 전수1검사/실패0을 확인했다(`run11-rawAltarSacrifice_first.log`). Dais 초기668도별도1검사/실패0이며 Altar2866에합산하지않는다.

단위11 최종 Release에서 경고0/오류0, 제단1·다이스1을 포함한 새15검사와 전체1038검사/실패0을 확인했다. 현재2866 자료/검사/생산 전체 SHA와 successor 감사도 최종 종합 감사에서 통과했다. 수명1928과 후속 상위 관찰은 이번2866에 더하지 않는다.

## 초기 관찰의 보존과 같은입력 재관찰

- 최초8/7 내보내기·30행 smoke·당시 production/oracle/spec/fixture/report는 `cpppj/hist/altarsacrifi-af04e`에 당시 SHA 그대로 보존했다. 전수1056번째의 미허용 중심helper 중단은 정상전수로 합산하지 않는다. 가상표 초기화와 원본Player 전역의 실제 배열포인터 해석 교정 중의 진단 실패도 별도 로그로 유지한다.
- 중심9/8의 첫전수2866·당시 전체tool/fixture/report/production은 `cpppj/hist/altarsacrifi-6e4d0`에 보존했다. 실제1칸SHP와 이미1단계버킷에 있는 slots5/6/7의 초기 levelBYTE를0→1로 맞춰 같은2866 입력을 다시 관찰했다.
- `analyze_altarsacrifice_successor.py --verify`와 `altarsacrifice-successor.json`은 production 전체바이트 불변, 역사파일의 당시SHA, 같은입력키와 모든 반환/효과/RNG/player 결과 불변을 증명한다. raw 차이는 위 초기BYTE3개뿐이다. 최초30/실패시도/재실행은 더하지 않고 현재 정상2866만 유지한다. 기존 Dais668 및 기존 helper 관찰도2866에 합산하지 않는다.

## 남은 실제 소비자

제단 가상삭제는 자체preDestroy에서 같은 살아있는다이스를 동일flags로 지우고 주변 abstractRune를 정리한다. `RawDaisLifecycle` 초안은 그 실제몸체와 Dais/Altar 최초Pop/Player SID 쌍 등록/삭제정리를 작성했다. Dais 최초Pop은 실제AltarFactory→owner→전체Pop를 수행하고, 제단 최초Pop은 실제로컬21을 예약한다. 이5/5의 **1928정상/assert0** native와 C++ 전수초안을 단위12 후보로 별도준비했다. 각판본964입력/두x87반환1928이며 마지막전체Damageable 등효과경계는분리한다. C++ 전수/전체소비자 대조와 SquidPostPop의 실제 등록은 아직 실행전이며11에는1928을더하지않는다.

마지막 **DamageablePostDestroy0044b9e0/CD00461b40 전체**는 현재 미복원이며 root가 별도복원을 준비한다. 기존 SquidDestroyLifecycle의 공통post로 대신하지 않는다. 실제 Altar→Dais 삭제 및 사제 해방의 전체 같은Pool·Host 소비자, Dais Progress/Bridge/Rune304 전체 원본 관찰·실제 음향/UI/GameWorld 연결과 캠페인1-1 의식 완주는 미완료다.
