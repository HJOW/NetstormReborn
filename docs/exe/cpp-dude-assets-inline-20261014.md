# 공유 Dude 자산 검사 헤더의 inline 교정

현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 패치 **10.78**과 CD **10.72**의 기존 Native/body/fixture는 그대로 유지했다. 단위14 세번째 완성 Release 빌드에서 `PlayerWorldTests`와 기존 `DudeDropTests`가 같은 `DudeSpawnSupport.h`를 포함하여 `Assets` 함수의 **LNK2005 중복 정의**가 발생했다.

교정은 해당 함수 앞에 **`inline` 한 토큰**을 넣는 것뿐이다. 함수 본문·주석·원본 자산·서버 Native·모든 fixture는 바꾸지 않았다. 같은 함수의 지역 static 자산을 여러 TU에서 사용할 수 있도록 정의를 연결한다. 새 검사/원본 입력은0이다.

교정 전에 현재 Support 전체·기준13 aggregate/Spawn/Drop 보고서와 draft lineage·현재 Player Native/실제Attach/공유lineage/검사·세번째 완성 빌드 로그 및 참조 whole자료를 `hist/dude-assets-e44e0`에 보존했다. 보고서 참조를 재귀 추적한 보존기가 parent의 최소 범위 지침 도착 전에 이미 끝나 **작업 whole3954 + 기준13 Git1 + capture1**이 남았다. 이후 역사 재귀복사는 추가하지 않았다. 기존 보고서는 덮지 않았다.

현재 별도 successor는 `dude-assets-inline-lineage.json`이다. 감사 도구 `tools/analyze_dude_assets_inline_lineage.py`가 다음을 확인한다.

- 현재 inline 한 토큰을 제거하면 이전 작업 **전체 바이트**로 정확히 돌아간다. 기준13 Git LF와 작업 개행 원문도 따로 검증한다.
- 이전 단위13 전체 **1066** 근거는 당시 Player 등록 전 source/binary/log view로 확인한다. 현재14 바이너리로 바꾸어 검증하지 않는다.
- Player 분배만 정확히 역제거한 두 공유 소스와 inline 제거가 확인된 Support를 **읽기 view**로 공급하여 이전 Dude288/Drop308/draft 감사를 재현한다. 실제 파일은 바꾸지 않는다.
- 현재 Player 정상1452/assert228·고유1680·실제Attach80 보강과 현재 공유 Player lineage도 보고서 변경 없이 감사한다.
- Spawn288, Drop 정상306/assert2 및 Player1680 fixture/생산/도구는 불변이며 **새 Native0**이다.

```text
python -X utf8 tools/analyze_dude_assets_inline_lineage.py --verify
```

현재 감사는 통과했다. 네번째 공유 Release 빌드는 root가 시작했으며 **교정 후 C++ 재빌드/집중/전체 성공은 아직 미확인**이다. source/header/test는 빌드 종료까지 동결한다. 공유 CMake·다른 경고 교정·빌드/검사·상세 LEFT/commit은 root가 담당한다.
