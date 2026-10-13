# 다이스 상위 진행과 실제 알터 지연 입력

- 현재 PC와 마지막 원본 내보내기: **HJOW-Athlon, 2026-10-14**.
- 10.78 복원을 기준으로 10.72의 대응 주소와 분기 차이를 함께 기록한다.
- 단위 11의 `RawDaisRegular`, `RawAltarSacrifice`, `RawDaisLifecycle` 생산 파일과 앞 668/2866 관찰은 유지한다. 이 문서는 후속 단위 12 자료다.

## 새 관찰 범위

| 몸체 | 10.78 | 10.72 |
|---|---:|---:|
| 다이스 진행 | `00449f40` | `00482a20` |
| 룬 304 | `0044a3a0` | `004846b0` |
| 전역 169 진행과 감김 306 | `0044a200` | `00482df0` |
| 알터 20~24 | `0044a5e0` | `00483800` |
| 음향 조회 요청 | `004a8ee0` | `00437d20` |
| 음향 프로세스 생성 요청 | `004ab240` | `00453800` |
| 벽시계 요청 | `00460d70` | `004011b0` |

`Progress`의 실제 스택 정리는 두 판본 모두 4바이트다. Ghidra가 사용하지 않는 룬 SID 인자를 생략한 선언을 그대로 호출하면 반환 스택이 맞지 않는다. 음향 생성은 ECX에 새 프로세스 메모리를 전달하는 thiscall이며 `(parent SID, sound, 0, 12)` 네 인자를 16바이트 정리한다. 벽시계는 cdecl 출력 포인터 하나를 받고 그 포인터를 반환한다.

다이스 진행은 최초 stage를 보존하고 실제 강한 사제 조회·현재 K~O 중지·붕괴 또는 완료 음향·포획 내용물 조회·희생 시작/알터 이탈을 실행한다. 두 음향의 할당 성공과 부착 성공은 독립 입력이다. 부착 성공 뒤에만 벽시계를 읽어 원본이 프로세스 `+20`의 double에 쓴 시각을 기록한다. 벽시계 입력은 123.25이며 결과 시각을 기대 코드로 계산하지 않는다.

앞 알터 관찰은 payload 9.25를 사용했다. 새 자료는 실제 이탈 예약값 0.12와 0/0.08/1의 20~24를 별도 원본 호출로 관찰한다. 24의 음향 확률 분기·프레임 감김·가상삭제 요청·3.0 반환과 난수 상태를 원본에서 읽는다. 일반 FrameAdvance/typed/contained/form 조회와 Finish는 실제 원본 명령이며 가상 공간·생성·파괴·UI·음향 장치는 명시 경계다.

## 자료와 현재 상태

- 도구: `tools/decomp_daisprogress_oracle.py`.
- 기대 입력/출력: `cpppj/tests/fixtures/daisprogress-x86.tsv`.
- 현재 출처: `cpppj/recovery-daisprogress-evidence.json`.
- 전용 C++ 검사: `cpppj/tests/DaisProgressTests.cpp`. 단위12 첫Release경고/오류0이며새Progress1검사/2964전수/실패0을확인했다.
- 정상 **2964개/assert0**, 판본별 1482개를 두 x87 정밀도에서 같은 출력으로 관찰했다. 판본별 입력 구성은 Progress216·Rune304114·감김30696·알터 payload1056이다. `daisprogress-native-focused.log`에 완료를 기록했다.
- 원본 보고서를 보존하고 새 검사 전체 SHA를 `analyze_daisprogress_cpp_binding.py`/`recovery-daisprogress-cpp-binding-evidence.json`으로 별도 연결한다. 이 연결의 새 원본 입력 추가는0이다.
- 처음검사의중복`results`선언하나를지웠다. 그전전체6파일을 `daisprogress-before-cpp-declaration`에보존했고 `analyze_daisprogress_cpp_correction.py`/`recovery-daisprogress-cpp-correction-evidence.json`으로정확역변환·생산/원본2964/tool/fixture/옛연결보고서불변과현재TestSHA를연결한다. 추가원본0이다.
- 최초 준비 중 기존 공통 `self.g`를 새 스펙으로 가린 오류와 Progress의 실제 ret4를 확인해 교정했다. 해당 실패는 정상 관찰에 포함하지 않는다.
- 전수 첫 시도는 서버 stage0~3의 자격을 충족한 사제 이동이 실제 Path mapper `00488fe0`로 들어가 미허용 주소에서 중단됐다. 로그를 보존하며 해당 입력은 이 좁은 자료에서 제외했다. 빈 Path 경계로 완료 처리하지 않는다.
- 두 x87 제어 워드 0x027f/0x037f의 동일 입력 결과, 정상 EIP/ESP/SEH/FPCW/TOP와 현재 전체 파일 SHA를 확인한다. 앞 668/2866·수명 1928·기존 Path/룬/신전 관찰을 새 입력 수에 더하지 않는다.

## 실제 의식 소비자 연결에 필요한 범위

1. 같은 `SquidPostPop`에 다이스/알터 파생 `RawDaisLifecycle`을 등록하고 실제 `SquidPop`·`SquidFactory`·`SquidOwner`를 연결한다. 알터 최초 Pop의 로컬 21 예약도 같은 Host/Kernel/Form에서 수행한다.
2. 알터 24 → 다이스 가상삭제 `400000` → 실제 다이스 전처리와 각 알터 24의 Finish → 알터 가상삭제 `48` → 실제 알터 전처리/전체 Damageable 후처리 → 플레이어 SID 쌍 정리 → 공통 Unpop/SID 반납을 같은 Pool에서 이어간다.
3. 포획 내용물 type6은 기존 `ContentFormLifetime`과 `ContentFormHost`를 재사용한다. 실제 VT `00512208`/CD `00505b00`의 전처리 `004aeeb0`/CD `004b0300`, 후처리 `004ac1b0`/CD `004aba90`를 일반 자산 장부로 대체하지 않는다. 이 모듈은 커서 취소·패치 너겟 효과·삭제 깊이 반납·실제 Form Unpop/Release를 이미 담당한다.
4. 마지막 전체 `RawDamageablePostDestroy`는 별도 담당자가 복원한다. 최초 드롭 `0044b830`/CD `00461970`는 기존 해방 몸체와 다르다. 현재 희생 삭제 flags는 `flags & 0x300000 == 0`이므로 첫 Dude 생성 분기를 실행하지 않는 입력을 명시하고, 실제 일반 후처리와 같은 공통 삭제 수명을 연결한다.
5. 사제 해방은 같은 Factory의 158 생성·Owner·전체 사제 Pop·기존 forcefield/effect·포획 ContentForm 삭제까지 필요하다. 기존 `RawPriestPostPop`과 Tail/CarrierPostPop을 재사용한다.

실제 Path를 만드는 BridgeCommand, 실제 음향 장치, 의식 전체 같은 Host/Pool 소비자와 캠페인 1-1 완주는 아직 완료되지 않았다.
