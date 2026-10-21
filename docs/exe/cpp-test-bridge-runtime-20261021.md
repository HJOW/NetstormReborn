# 테스트 전용 다리 지정 공급과 실제 구동 준비

사용자가 필요한 다리 조각이 나오지 않아 신전이 파괴됐던 구동 실패를 설명하고 테스트 전용 지정 공급과 Devastation·사제 포획·희생 의식 검증을 요청했다. 이 요청 범위에서 기존 실행 이월 지시는 해제됐다. 시작 HEAD는 `b01e3ca7`, 현재/마지막 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**이다. 새 디컴파일·원본/CD/AGENTS 수정은 없다.

## 사용법

```powershell
.\cpppj\build\bin\Release\NetstormCpp.exe --run .\originals --mission tutorial5 --window --test-bridge-patterns 4
```

`--test-bridge-patterns 4`는 매번 `ㅏ`자 조각(`J./BK/J.`)을 공급한다. 3은4칸 직선, `0,3,1`은 한 칸/4칸/2칸 직선 순서다. 번호는 `0~25`이며 나머지 모양은 `--inspect-canon-patterns .\originals`로 확인할 수 있다. 옵션을 생략하거나 `off`로 지정하면 원래 공급 규칙을 사용한다. 회전·배치·재고 슬롯·생성 타이머·품질 회복은 기존 경로를 사용한다. 설정 파일에 저장하지 않으므로 다음 일반 실행에 남지 않는다. Early5에서는 다리를 과도하게 지어 캐논의3×3 건설 공간을 채우지 않도록 [수정 방어 입력](cpp-combat-draw-and-defense-20261021.md)을 사용한다.

UI 입력 스크립트에서는 `프레임<TAB>test-bridge-patterns<TAB>3,0,1`로 새 순서를 설정하거나 `off`로 해제한다. 이미 생성된 재고는 바뀌지 않고 다음 공급부터 적용된다. 새 목록을 설정하면 테스트 공급 횟수만0으로 돌아간다. 미션 전환/재그리기는 설정된 순서를 유지한다.

**이 기능은 테스트 전용 임시 조작이며 원본 게임 분석 결과가 아니다.** 구현·공개 선언·새 검사에 이 한국어 주석을 명시했다. 원본 난수 소비와 순번/강제 한 칸 조각 계산을 그대로 실행한 뒤, 실제 생성 직전 최종 패턴만 지정 값으로 바꾼다. 따라서 지정0도 원본의 강제 패턴0 해제 의미와 구별된다. 일반 실행의 기본 경로에는 지정 값이 없다.

실행 로그에 테스트 모드 문구를 남기며 활성 모드의 보고에는 `test_bridge_patterns<TAB>실제공급횟수<TAB>지정번호들`을 기록한다. UI `report`는 프레임마다 파일에 보존해 중단/오류 때도 직전 관측을 잃지 않는다.

## 검증

최종 Release 빌드 `extracted/cpp-test-bridge-20261021-build2.log`는 **종료0·경고0·오류0**이다. 첫 빌드의 새 optional/정수 비교 경고를 unsigned 상수로 수정했다. 집중 검사8개는 같은 최종 검사 바이너리에서 모두 통과했다.

- `ProductionMaintenance_` 4개: 새 순환/0번/해제/잘못된 설정 원자성/최종 우선순위/빈자리/원래 난수 소비와 기존 원본86행·발자국18행.
- `DataManagerRead_` 3개: 원본144행과 mark/이름 표 계약.
- `ReadTargetElevenFiles` 1개: 실제 목표11파일의35개 섹션·목록·현재 버퍼/빌림.

실제 클론 GUI 실행 `extracted/test-bridge-20261021-supply1/`은 `thewarbegins`, 1024×768 창, 지정 `0,3,1`로 브리핑의 Play Mission을 눌렀다. 초기1개→중간4개→최종6개 재고를 관측했고 같은 실제 내용물 패턴이 **0,3,1,0,3,1**이었다. 실제 공급 횟수6, 슬롯6, SP3000을 유지했다. `full.bmp`를 열어 왼쪽 패널과 실제 게임 화면을 확인했다. 종료0·보호 원본 전체 해시 동일이다. 이 실행은 주문/포획/희생 성공 증거가 아니다.

입력은 `cpppj/tests/fixtures/test-bridge-supply-ui.tsv`, 재실행 도구는 다음과 같다. 출력은 존재하지 않는 새 폴더여야 한다.

```powershell
python tools/cpp_bridge_ritual_runtime.py --plan cpppj/tests/fixtures/test-bridge-supply-ui.tsv --mission thewarbegins --patterns 0,3,1 --frames 393 --fps 60 --output extracted/my-new-bridge-run
python tools/cpp_bridge_ritual_runtime.py --filter CampaignDevastation_ --output extracted/my-new-devastation-tests
```

실행기는 입력·PID·바이너리 SHA·stdout/stderr·종료·설정 복구/원본 해시를 각각 보존한다. 성공 결과로 과거 실패나 원본 기대값을 덮지 않는다.

빌드 입력의 실제 작업 트리 바이트 해시와 Git의 줄바꿈 정규화 후 커밋 바이트 해시를 구분해 기록한다. 첫 스테이징 확인에서 CRLF→LF 차이를 발견했으며, 빌드/실행 입력 해시를 지우지 않고 `committed_source_hashes`를 추가했다. 실행 로그·바이너리·기존 관측 결과는 그대로 보존한다.

## 이어서 수행할 작업

이 준비 단위를 `20261021-34 CP`로 로컬 커밋한 뒤 미뤄둔 Devastation·포획/Drop·제단 배달·희생 의식의 집중 검사와 실제 GUI 진행을 이어간다. 현재 준비만으로 세 동작이 실제 GUI에서 성공했다고 판정하지 않는다. 기존 Early5 입력의 적 HP/좌표/SP/미션 단계를 쓰지 않고 지정 다리 공급만 켜서 진행한다. 일반적인11미션 완주·원본 동일성 목표도 계속 active다. 다른 세션의 dotnetpj 작업은 건드리지 않고 push하지 않는다.
