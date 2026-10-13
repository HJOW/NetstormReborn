# 사제 의식 프레임과 후속 예약 — 2026-10-13

마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-13**이다. 같은 정밀 프로젝트의 `priestritualactions` readOnly 내보내기 **10.78 네 함수 / CD10.72 세 함수**를 사용했다. 보호 PE와 AGENTS.md를 보존했다.

## 복원한 몸체

| 역할 | originals 10.78 | originalCD 10.72 | 실제 C++ 연결 |
| --- | --- | --- | --- |
| 의식 사제 첫 프레임 | `00491ca0` | `0040cf10` | `RawPriestRitualActions::Begin` |
| 프레임 뒤 후속604 예약 | `00491cf0` | `0040cf60` | `RawPriestRitualActions::Continue` |
| 현재 타입 조회 | `004ac550` | `004abf50` | 같은 SidPool raw BYTE10 |
| 타입 번호 조회 | `0049a840` | 현재 타입 getter에 인라인 | 실제 타입 표 인덱스 |
| 프레임 setter | `004acee0` | `004acbc0` | 기존 `SquidFrame::Set` |

Begin은 SID1..capacity-1의 현재 raw 타입을 읽어 **K+stage의 첫 프레임**을 flags0으로 요청한다. free/dead/void는 추가 생략 조건이 아니다. 실제 타입 first-table의 K..P에 대응하는 stage0..5를 받는다. 전체 신전 의식 본문이 사용하는 정상 단계는0..4다.

Continue는 유효 번호 검사→Begin→new0x28 요청→성공 시 **같은 사제/event604/payload0.01F**의 SharedRegular 순서다. 프레임 callback 뒤 상태나 타입이 바뀌어도 처음 유효했던 번호의 예약을 계속한다. 기존 같은604 예약을 취소하지 않는다. `MakePriestRitualActionHooks`는 같은 풀의 기존 SquidFrame과 Host.AddSharedRegular를 연결하고 예약 확보 경계는 caller가 공급한다.

## 독립 원본 대조와 입력 교정

두 보호 PE의 실제 wrapper/type getter를 Unicorn으로 실행한 **정상1752개 = 판본별876개**, assert0개다. SID5/23999, stage0..5, 실제 LetterRun 길이1..3, 현재 타입133/158, 확보 성공/실패, frame 뒤 타입 변경과 확보 뒤 raw 상태 변경, SID0/24000/0xffffffff를 관찰한다. x87 정밀도 반복/동일 입력 반복은 신규 합산에서 제외한다. 관찰 경계는 frame setter, malloc, SharedRegular ctor이며 전체 신전 의식 완료를 주장하지 않는다.

`PriestRitualActionsTests.cpp` 첫 검사는 원본의 효과 순서와 vtable 주입 DWORD를 제외한 raw 슬롯 전체를 전수 대조한다. 초기 C++ 입력 준비에서 client SID5를 서버 영역 전용 Take로 확보해 실패한 것을 교정했다. 현재는 SID5를 **Create(type,flags2)**로, 서버 SID23999만 Take로 확보한 뒤 원본 fixture와 같은 raw 바이트를 작성한다. native/fixture/production 바이트와 신규 관찰 수는 바뀌지 않는다.

두 번째 검사는 실제 type158 생성자·LetterRun→SquidFrame→같은 Host/Kernel의604 예약을 연결한다. 같은 hash 단계의 frame 두 갱신, 정확한 payload/시각과 같은604 두 예약의 공존을 검사한다. 이 검사는 사제 Pop 전체를 실행하지 않는다.

Factory include 누락, 없는 RiftTypeRecord.number 대입, client/server SID 준비 교정 전 test/report를 각각 `extracted/priestritualactions-before-*` snapshot으로 보존했다. `tools/analyze_priestritualactions_test_lineage.py`는 세 수정만 되돌리면 초기 전체 테스트와 같고 기존 production/fixture/PE/export/oracle가 그대로임을 증명해 현재 SourceSHA를 연결한다. 교정의 신규 원본 관찰은0개다.

단위07 최종 공유 Release 두 타깃은 **종료0·경고0·오류0**, 집중 C++ 두 검사 모두 성공했다. **1752개 전수 대조**와 실제 frame/같은 SharedRegular 예약·중복 공존 검사를 통과했다. **전체983개 검사/실패0**이며 통합 근거는 `run7.json`, 초기/교정/최종 로그는 `extracted/run7-*`에 보존했다.

```text
python -X utf8 tools/decomp_priestritualactions_oracle.py --verify
python -X utf8 tools/analyze_priestritualactions_test_lineage.py --verify
```

## 남은 범위

신전 의식 `00447e40/CD00481fc0`의 현재 단계/두 목표 위치/룬 조회, 사제 강약/소유자 후보, 단계별 음향·event0x307 예약, 룬 생성/추상화/owner/Pop, 후속 신전 사건 처리와 사제 운반·실제 GameWorld/UI 연결은 후속이다. CD10.72는 단계 helper로 분리되며10.78은 상위 의식에 같은 단계 계산을 인라인한다. 양판본 주소/분기/폭을 후속 기록에서도 유지한다.
