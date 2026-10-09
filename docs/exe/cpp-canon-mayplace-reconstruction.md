# 일반 배치 전체 MayPlace 연결과 독립 원본 대조

2026-10-09, 마지막 디컴파일 수행 PC: **HJOW-Athlon**. [이전 최종 관계 복원](cpp-canon-relations-reconstruction.md)의 모듈들을 실제 전체 함수 진입부터 정상 반환까지 연결했다. 게임이나 창을 실행하지 않고 읽기 전용 Ghidra 내보내기·제한 기계어 실행·콘솔 검사로 확인했다.

## 복원 모듈의 공통 자료 연결

[`RawCanonPlacementPipeline`](../../cpppj/src/o/RawCanonPlacementPipeline.h)은 픽셀 getter → decoder → 로컬 미리보기 → 일반 finder → 충돌 → 지형 → 실제 Player 기준점/주변 권한 → 최종 소유 관계를 연결한다. 후반 허용 정책을 임시 true로 공급하지 않는다.

현재 표면 조회·일반 finder·Player 그래프 조회·미리보기·최종 관계는 같은 `SquidHash::Entries(0)`을 읽는다. 고정 섬 지도는 별도 입력이다. 공통 관계/편집기/그래프 준비/noIsland 전역을 호출 직전에 동기화한다. 관계 원본은 `permission`, 준비 상태는 `permission.graphReady`, 받침 번호는 `geometry.patternTypes[3]`이다. 자료 span·상태·풀·장부는 호출자가 소유하며 파이프라인보다 오래 살아야 한다. 내부 훅이 모듈 주소를 참조하므로 복사와 이동을 금지했다.

## 전체 함수 원본 관찰

[`canonmayplace-functions.json`](../../tools/ghidra/canonmayplace-functions.json)은 앞 단계의 실제 helper 목록을 합친다. `originals` 56개, `originalCD`/추가 10.37 각각 51개를 이 PC에서 새로 내보냈다. [`decomp_canonmayplace_oracle.py`](../../tools/decomp_canonmayplace_oracle.py)는 패치 `0049b510`/CD `00445200`부터 **ret 24 정상 반환**까지 실행한다. 픽셀 getter·decoder·finder·충돌·지형·Player·주변·관계 몸체를 대체하지 않는다. **임시 160바이트 확보/반납 진입만 대체**하며 메모리 부족·예외 전파는 검증하지 않는다.

세 PE 각각 144개, 총 **432개**를 두 x87 제어 워드 `0x027f`/`0x037f`로 실행했다. 입력은 일반 8종 genus × 6장면 × 2권한 문맥 96개, 실제 3×3 받침의 네 정상 회전/4장면/2로컬 소유자 32개, 좌표 경계/강제 허용 16개다. 지면·외국 소유·구멍·다른 단계 충돌·빈 지도·누락 작업장·특수 지역 제한을 포함한다. 받침 noIsland의 정상 인자는 0이며 다리의 인자 25와 혼동하지 않는다.

정상 반환의 EAX/ESP·EBX/ESI/EDI/EBP 보존·FS 복원·x87 제어 워드/TOP·허용 쓰기·임시 메모리 균형을 확인한다. 허용 결과·표시 차단·144바이트 미리보기/지역 배열·raw/현재 해시/고정 지도 checksum을 저장한다. 각 PE 전체 진입/반환 288회, 임시 확보/반납 각각 104회이며 실제 helper 진입 종류는 패치 49종, CD/10.37 각각 41종이다. assert/OS 호출은 0이다.

[`canonmayplace-x86.tsv`](../../cpppj/tests/fixtures/canonmayplace-x86.tsv)와 [SHA 근거](../../cpppj/recovery-canonmayplace-evidence.json)를 LF로 저장한다. C++의 새 검사 3개는 세 PE 전체 행을 동일 입력의 실제 파이프라인으로 재생한다. 새 fixture 432개를 더해 누적 **341,842개**다. 기존 fixture와 감사 도구는 수정하지 않았다.

## 실제 TYPE·SHP 연결 검사

콘솔 명령 `--inspect-canon-mayplace <game-dir> [--cd]`는 실제 타입 정의·프레임 코드·물리 SHP·hotspot·패턴 자료를 파이프라인에 넣는다. [`cpp_canonmayplace_smoke.py`](../../tools/cpp_canonmayplace_smoke.py)는 자산 파일을 독립 파싱하고 실제 원본 hotspot 변환과 전체 MayPlace로 기대값을 얻는다. C++ 출력은 결과 비교에만 사용한다.

대표 요청자는 `sunArcher`, `sunBlocker`, `sunFactory`, `priest`, `bridge`, `noIsland`, `island` 7종이다. 일반 자산 4종과 패턴의 정상 인자/네 회전을 정상 지면·충돌·지도 끝 안쪽·외국 소유자의 4장면에 넣는다. 두 판본 각각 **480개**, 총 **960개**에서 허용/차단/미리보기/지역 배열을 비교한다. 이 값은 합성 fixture 누적 개수에 포함하지 않는다. 주변 지형·작업장·충돌자는 자산 번호와 겹치지 않는 내장 타입 10/11/12의 합성 입력이다. 실제 저장 맵이나 생성/삭제 수명 검사가 아니다.

초기 검사에서 관찰 도구의 사제 프레임 주소 표와 헤더가 겹쳐 원본 여백이 잘못 계산되었다. 실제 자산용 헤더 영역을 분리한 뒤 재검사했다. CD의 긴 다리를 `(254,254)`에 놓으면 원본 미리보기의 선형 spot 읽기가 지도 밖으로 나가 C++의 범위 검사에 걸린다. 자산 대조의 공통 경계는 `(250,250)`으로 제한했다. 임의 지도 밖 메모리를 공급하여 원본의 정의되지 않은 읽기를 정상 동작으로 주장하지 않는다. 기존 합성 전체 함수의 정상 경계 입력은 그대로 유지했다.

## 재현과 남은 범위

```powershell
tools/ghidra/export_functions.ps1 -Name canonmayplace
python -X utf8 tools/decomp_canonmayplace_oracle.py
python -X utf8 tools/decomp_canonmayplace_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_canonmayplace_smoke.py
```

최종 검증 결과와 시간은 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 이번 완료 항목을 따른다. 로그는 `extracted/canonmayplace-export.log`, `canonmayplace-oracle-final.log`, `canonmayplace-build-final.log`, `canonmayplace-ctest-final.log`, `canonmayplace-audits-final.log`, `canonmayplace-assets.log`와 `cpp-canonmayplace-assets-report.json`이다.

다음은 **실제 저장 맵/raw 세계·공간 장부·Player 별도 추가 목록의 생성/삭제 수명 연결**이다. 이어 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사와 GUI 건설·경제·전투·승패가 남았다. 전체 함수의 제한 입력 대조 완료를 자산 전수·미션 완주·플레이 가능한 게임 완성으로 해석하지 않는다. 여러 판본 자산 전수·장시간 변이·최대 지도·창/픽셀 회귀·0 발자국/비정상 입력 전수는 후속이다.
