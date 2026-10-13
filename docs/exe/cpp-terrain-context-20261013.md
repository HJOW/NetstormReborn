# 초기 지형 context 복원

작업 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-13**이다. AGENTS.md와 두 인수인계 문서를 다시 확인했다. 보호 원본 PE는 변경하지 않았으며 게임·GUI 실행은 없다.

## 실제 주소와 저장소

| 몸체 | 10.78 | CD 10.72 |
| --- | --- | --- |
| constructor | `004c0390` | `0049cdd0` |
| 청크/칸 bounds | `004c02d0` | `0049cd50` |
| 이웃 불일치 문자 변환 | `004c09a0` | `0049d890` |
| 도장 뒤 문자 정리 | `004c0ad0` | `0049d9a0` |

`RawTerrainContext`가 code/region 두 65,536BYTE 배열과 문자별 mapping/alias 256DWORD를 소유한다. 인덱스는 `x*256+y`다. `Map()`의 span은 이 저장소를 참조하므로 복사·이동을 금지한다. constructor는 code를 `.`로, region을 현재 noRegion 하위BYTE로 채운다. `x/=`는 isle97, `X`는 edge98, `./o`는0이며 `x/X/o/=`의 alias는 모두 `x`다. 실제 특수 타입 값은 외부 전역으로 주입한다.

청크 bounds와 16배한 칸 bounds는 따로 저장한다. right/bottom 청크 초과에서 **청크 값만16으로 줄이며 이미 저장한 칸 bounds는 보존**한다. 10.78은 해당 assertion을 보고하고 CD는 조용히 줄인다. 중립 owner0의 fringe 제외값0x18은 battle의0x10보다 우선한다.

`MarkEdges`는 Y외부/X내부 순서로 여덟 이웃의 inclusive bounds·현재 alias·지역을 조사해 첫 불일치에서 문자를 바꾼다. 앞에서 바뀐 `=`도 같은 alias 표로 읽는다. `NormalizeAfterStamp`는 같은 순회에서 `=/o/X`만 `x`로 바꾸고 지역은 보존한다.

## 원본과 C++ 대조

보호 두 PE의 실제 몸체를 분리 메모리에서 실행한 **224개 고유 입력(정상170·assert54)**을 `terraincontext-x86.tsv`에 기록했다. 판본마다112개이며 constructor72·경계20·정리20이다. C++ 검사는 모든 입력의 owner/context·청크/칸 bounds·전체 mapping/alias·uniform 초기 배열·16×16 변경 창·assert 횟수를 대조한다. 같은 저장소 참조와 필수 진단 누락·배열 밖 접근 거부도 확인한다.

- 원본 실행기: `tools/decomp_terraincontext_oracle.py`
- 출처 연결: `tools/analyze_terraincontext_cpp_binding.py`
- 보고서: `cpppj/recovery-terraincontext-evidence.json`
- 첫 C++ 집중 검사: **2개 통과, 실패0**
- 최종 공유 빌드/전체 검사: 단위08 종합 보고서에 기록한다.

첫 native 실행에서 빠진 CD 이웃 helper `0049d580`을 발견했다. 실패 로그를 보존하고 현재 호스트에서 해당 함수만 읽기 전용으로 추가 내보낸 뒤 전수 실행을 완료했다. 실패 실행·반복 실행을 새 고유 입력에 더하지 않는다.

## 남은 범위

원본의 ChunkMap 포인터 주소·객체 metadata 복사는 게임 데이터 비교에서 분리했다. 이 단위는 실제 성장·일반/archipelago wrapper·edgeFarm 군집 배치·게임 월드·캠페인 완주 완료 근거가 아니다. 도장의 실제 월드 소비자는 [지형 월드 연결](cpp-terrain-world-binding-20261013.md)을 따른다.
