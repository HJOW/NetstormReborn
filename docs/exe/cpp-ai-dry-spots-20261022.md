# AI 에너지 부족 좌표 목록과 재검사

2026-10-22 단위286, 시작 HEAD `7c51717d / 20261022-08 CP`. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-21**로 같다. 기존 정밀 원문과 보호10.78 PE를 정적으로 읽었다. 새 디컴파일·원본/CD/AGENTS/dotnetpj 변경과 게임·집중 C++ 검사 실행 재개는 없다.

## 원본 의미와 저장소

이전에 잔여 목록에서 “고갈 지점”이라고 적은 `0040f2b0`는 자원 고갈 간헐천 정리가 아니다. `00417c70`의 **Dry Spot**은 에너지가 부족했던 배치 좌표를 한 항목씩 다시 검사하는 내장 목록이다. 현재 `004734d0` 에너지 판정이 비영이면 해당 기록을 제거하고 TYPE를 반환한다. `00415fe0` 등 가짜 좌표 선정/다른 전략의 기록 생성은 후속이며, 이번에 전체 자동 전략을 활성화하지 않는다.

[`RawAiDrySpots`](../../cpppj/src/o/RawAiDrySpots.cpp)는 기존 생성자의 같은0xb08 본체와 실제 소유 backing을 사용한다. 필드는 capacity `+adc`, count `+ae0`, pointer `+ae4`, cursor `+ae8`, owner `+aec`이며 실제 생성자는 용량10/120바이트를 확보한다. 각12바이트 기록은 x float 비트/y float 비트/**TYPE DWORD**다. Ghidra가 `0040f1c0`의 세 번째 인자를 float로 표현했지만 실제 명령은 MOV로 DWORD를 그대로 보존한다.

## 복원한 흐름

- `0040f1c0 Add`: signed count가 용량 이상이면 포인터도 읽지 않는다. 여유가 있을 때 x/y만 float 동등 비교해 중복을 거부한다. TYPE가 달라도 같은 좌표는 중복이며 signed0은 같고 NaN 좌표는 서로 같지 않다. 중복이 없으면 원래 끝 기록에 x/y/TYPE를 쓰고 count를 증가시킨다.
- `0040f260 Remove`: 뒤12바이트 기록을 x→y→TYPE 순으로 안정 압축한 뒤 count만 감소시킨다. 미사용 꼬리와 cursor는 지우지 않는다. 원본처럼 index가 끝 이상이면 복사 없이 count가 감소하며, count0/index0도 DWORD가 -1로 감긴다. 음수 인덱스/실제 블록 밖 접근과 IDIV overflow는 그 접근 지점에서 명시 거부한다.
- `0040f2b0 Scan`: count0이면 다른 필드를 읽지 않는다. 비영이면 signed `(cursor+1)%count`로 **한 항목만** 선택한다. backing/owner를 보관하고 cursor를 먼저 쓴 뒤 빈 Mana 문자열을 초기화한다. 같은 `RawEnergySupply::Evaluate`에 원래 좌표/TYPE/owner를 넘긴다. 충족하지 못하면 그대로 두고0, 충족하면 **효과 뒤 현재 pointer/cursor에서 TYPE를 다시 읽고** 그 기록을 제거한다. 제거 후 cursor 보정이나 “바로 옆으로 이동” 개선을 추가하지 않는다. TYPE0을 제거하면 반환도0이다.
- `00417c70` 해당 하위와 `004105b0`: 반환TYPE가 비영일 때 현재 `+280` fake TYPE와 비교하고 현재 `+b00` 디버그 플래그가 켜져 있으면 원본 `Killed [Fake] Dry Spot` 로그를 남긴다. 로그 전역 `0052f3dc`의 PE 초기값은99다. 현재 owner의 `AI%d: %s\n` 접두와 메시지 안의 개행도 유지한다.

Session은 별도 관찰용 목록 없이 현재 Player+b0의 실제 본체, 같은 에너지 공급/선택 목록·방향별 동맹·TYPE/AST 요구 문자를 사용한다. 명시 `RecordAiDrySpot`과 `RecheckAiDrySpots`를 추가했다. 본체가 없으면 등록false/제거TYPE0이다. 기존 생성자/소멸자가 같은 backing을 확보/반납하며, 새 소유 목록이나 자동 성공 에너지 판정은 만들지 않는다.

## 확인 기록

새9검사는 모듈7개+실제 에너지 공급1개+실제 Session1개다. 좌표/TYPE 비트·signed0/NaN 중복, full/empty 포인터 미조회, 한 항목 순환·제거 뒤 cursor, 안정 압축/꼬리/끝 인덱스·count 감김, 효과 뒤 backing 해제/교체·cursor/count/fake/디버그 재조회, TYPE0/원본 로그, 누락/물리 범위/나눗셈 오류의 앞선 쓰기 보존을 다룬다. 실제 공급자 목록을 바꿔 부족 좌표가 유지됐다가 충족되면 제거되고 같은 selected 목록을 쓰는 검사, 실제 Session의 본체/에너지/중복/AI 소멸 검사를 작성했다. **모두 작성·컴파일만 하며 실행하지 않는다.**

Release 빌드 세션35791의 실제 종료까지 조회했고 **종료0·경고0·오류0**이다. `extracted/cpp-ai-dry-spots-20261022-build1.log`와 `build1-result.json`을 보존했다. 로그는 디코딩한 문자의 개행/내용을 그대로 보존해 UTF-8 바이트로 변환하고 동등성을 확인했다.

[`cpp_ai_dry_spots_audit.py`](../../tools/cpp_ai_dry_spots_audit.py)의 [신규 정적 보고서](../../cpppj/ai-dry-spots-static-20261022.json)는 원본9함수의 정밀 범위/명령·저장 폭/순서·보호 원본 동일 SHA·최종 소스/바이너리 SHA·새9검사 링크와 같은 본체/에너지 연결을 확인한다. **C++ 검사·CTest·게임·native 실행0**이다. 이전 보고서는 덮어쓰지 않았다.

CD10.72의 Scan 대응 주소는 기존 매핑의 `004d9480`이며 별도 구현/실행 대조는 하지 않았다. 전체 x87 정밀도/예외, 자연 프레임에서의 목록 채움/제거·음향/화면은 미검증이다. 다른 AI 행동/원래 순서 자동 프레임·종료와 목표15개(Early Missions6+캠페인1-1..1-6+데모3), Edit/custom Fort·시나리오·전환/저장 수명/자연 플레이 동일성은 계속 남아 있다.

빌드/정적 기록/인수인계를 확정한 뒤 `20261022-09 CP` 형식으로 로컬 커밋한다. 원격 push하지 않는다.
