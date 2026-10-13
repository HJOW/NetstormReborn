# Dais 의식 소비자 준비: 공유 altar 조회와 이동 깊이 — 2026-10-14

기준은08 커밋 **552656c**이며 당시 Release경고0/전체998개 성공을 보존한다. 최신 Dais 순차 Refined readOnly 내보내기는 **패치18/CD10.72 21함수**, 종료0이다. 마지막 디컴파일 PC/날짜는 **HJOW-Athlon,2026-10-14**다. 기존13일 보고서와 보호 원본/AGENTS/dotnetpj는 보존한다.

## 승인된 기존 파일의 최소 변경

`RawRunePostPop::AltarAt(Sid)`은 기존 `ConstructionSeconds`의 typed altar 조회 prefix를 그대로 옮겨 공개한다. 동일한 현재 타입/genus·좌표·네 해시·첫 일치 순서를 사용한다. `ConstructionSeconds`는 이 API를 호출한 뒤 기존7/10/25 시간 선택을 그대로 실행한다. Dais는 같은 실제 조회를 재사용한다. Rune PostPop의 발견 Dais에서 시간을 읽는08 교정도 보존한다.

`RawPathRuntime::MoveAwayDepth()`와 `SetMoveAwayDepth(int32)`는 **helper+4의 signedDWORD**만 읽고 쓴다. 위치는 패치`0x186d8`/CD`0x2f18`이다. 기존 helper, 도착 targetA/B, 명령 꼬리, 목적지, 꼭짓점과 시각은 바꾸지 않는다. 원본 getter는`00427d30/CD004e7660`, 쓰기 caller는 Dais의`00449900/CD00481ab0`이다.

## 전체 기존 본문과 이전 관찰 보존

`tools/analyze_dais_shared_lineage.py`와 `cpppj/recovery-dais-shared-lineage-evidence.json`은 신규 Runtime 선언/정의4줄씩을 역제거하고 Rune의 새 선언/설명·위임을 되돌리면 **네 기존 파일 전체 문자/토큰이552656c와 같음**을 증명한다. 옮긴 typed 조회 prefix도 정확히 같다.

기존 Rune1920/경로 Runtime·PathStart·Cleanup/08통합·등록/교정 보고서의 바이트를 당시 git 객체와 비교한다. 현재 존재하는 PE/export/tool/fixture는 기준08 및 당시 보고서 SHA를 함께 기록한다. `extracted/pathruntime/*/creation.c/functions.tsv` 등 과거 별도 ignored 내보내기 일부는 현재 호스트에 없으므로 보고서에`available:false`를 명시한다. 그 파일의 현재 바이트를 검증했다고 주장하지 않는다. 원보고서와 현재 fixture/기존 전체 본문 lineage를 보존하며 새 Dais 관찰에 이전 수치를 합산하지 않는다.

이 API 준비의 **추가 고유 원본 입력은0**이다. 새 Dais 몸체/제한 PE/실제 소비자는 다음 단위에서 별도로 연결한다. 08 당시 성공을 현재 변경 소스의 새 실행 결과로 대신 쓰지 않는다.

```text
python -X utf8 tools/analyze_dais_shared_lineage.py --verify
```

## 이어갈 실제 사슬

Rune304→근처 Dais Progress→Rune flags0x48 삭제, Dais305 단계 진행/306 희생/307 의식 후속을 복원한다. 기존 TempleRitual/Frame/Carrier contained/Host/PathStart를 재사용한다. 다음 목표 이동은 마지막 꼭짓점을 다시 지정하고 현재 메타로Prepare한 뒤 깊이+1을 저장한다.

PE 실제 상수에서 희생 완료의 다음 첫 프레임은 count1→64/count2→120/count3→0이다. CD 획득 타입의 비로컬 분기는 패치의 SP/타입 전달과 다르다. CD localSP5000은 현재 값 getter→setter→표시를 사용하며 기존 `SquidReward::AddSp`의 AI지갑 효과를 넣지 않는다. 실제 player/UI/희생 종료·음향/경로 소비자와 전체 캠페인 의식 완주는 아직 미완료다.
