# 건설 출력1픽셀의 실제 인접 메모리 원인

- 단위: WB04-E1380, 2026-11-27, HJOW-Athlon.
- E1373의8000프레임 미확정1픽셀을 후속 Native 읽기 계측으로 확정했다. 제품 C++·원본·웹 표시 코드는 변경하지 않았다.

## 실제 저장 경로

격리 V5·V6 실행은 같은 캠페인1-1·고정25ms·실제 아군hard/적easy AI 입력을 사용했다. V6의 실제8000프레임에서 TYPE17 Form1302·부모SID15651·건설 표시token86이 화면(947,689)에 출력했다. 전체 캡처는49×55, hotspot(47,65), 화면 원점(929,649)이며 해당점은 상대(18,40), 선형1978이다.

입자285의 원래색은23, 수명 비트는1074126988(2.091830253601074초), 최대수명 비트는1073741824(2초)였다. 원문 색 조회가 최대수명으로 제한한 뒤 색수11과 곱해 인덱스11을 골랐고, 실제 읽을 수 있는12DWORD 중 배열 바로 뒤DWORD0이 output[1978]에 기록됐다.

| 현재 프레임 작성자 | 쓰기 전 | 쓰기 후 |
| --- | ---: | ---: |
| 섬 SID24476, 원색185 | 배경240 |185|
| 건설 중 TYPE72 부모 SID15651, 대상 색 변환 |185|232|
| 건설 표시token86, output[1978] 직접 복사 |232|0|
| 마나 볼트 SID1292, 그림자 마스크93 |0|0|

따라서 Native 검정은 이전 갱신의 잔류가 아니라 현재 프레임의 실제 건설 출력이다. 마지막 sprite 작성자는 마나 볼트이며 그림자표의 S(0)=0을 사용했다. 웹 기본 인접 입력1은 같은 경로에서 S(1)=11, 최종RGB(45,25,19)가 된다.

이전 정적 후보832는39×41 RLE 불투명 외곽의 인덱스를 전체 ShapeToBuffer 캡처 인덱스로 취급한 오류였다. 실제 SHP 헤더는49×55/hotspot47,59이고 TYPE hotspotY6이 더해진다. V6은 실제 Draw 필드에서1978을 다시 계산했으며 output[832]는255였다. [실제 원문](webpj-e1380-evidence/frame8000.tsv) · [필드 이름](webpj-e1380-evidence/trace-schema.json) · [관측값](webpj-e1380-evidence/proof.json).

## 적용 범위와 남은 판단

기존 웹 기본1의 근거는 원본x86 Demo3의 FieryTail TYPE11 공유11색을 독립 실행1회에서1257회 읽은 관측이다. 건설17의 보편 초기값이 아니다([원본 문서](exe/cpp-fierytail-live-heap-20261017.md)). C++ 건설 전용 ParticlePaletteMemory는 별도 private heap에서44BYTE를 할당하고 앞11DWORD만 쓰며, Snapshot이 그 바로 뒤4BYTE까지 실제로 읽는다. 관측0은 이 x64 할당 환경의 외부 메모리값이다. OS 내부 패딩/메타데이터 중 무엇인지 또는 모든PC에서0인지는 관측하지 않았다.

공통 기본1·그림자표·색수/수명 계산·기존 fixture를 바꾸지 않았다. 다음 수정은 Client/Session 적응계층의 종류별 명시 메모리 입력으로 분리해야 한다. 건설에0을 공급한다면 검증된 CPP1370 x64 할당환경 입력으로 표시해야 하며 원본 건설 상수 복원으로 쓰면 안 된다. 고정0/1 일반화·마지막색 clamp·좌표/SID/프레임별 보정은 하지 않는다. [소유권과 정책 근거](webpj-e1380-evidence/policy-analysis.json).

## 비간섭과 재검증

V5·V6 모두1000/6000/7000/8000의 제품 C++ 기준 화면0픽셀이다. 상태 원문은 SP소유자0 한 칸만 달랐고 기존SP0 제외 계약 뒤 모든 행0차이이며 Player 주소 잔재 행도0차이다. 보호5499파일의 변경·추가0, 두 실행 모두 정상 종료했다. 계측 당시 기준 제품 EXE SHA-256은 c3daa5bfbebfbcb91ab1b95965f9cecb745e17ec1110e9b574080229227249b7 이다. [V5 비교](webpj-e1380-evidence/v5-noninterference.json) · [전체 비교](webpj-e1380-evidence/noninterference-full.json) · [화면/상태](webpj-e1380-evidence/noninterference.json) · [보호 결과](webpj-e1380-evidence/protection.json) · [실행 출처](webpj-e1380-evidence/source.json).

오프라인 도구는 Native TSV의 실제 캡처/입자/쓰기 필드, 원본 SHP 헤더, 현재 GIFCLOUD 팔레트, 실제 건설 색 조회와 마나 볼트 그림자를 대조한다. 명시 인접0/1의 두 결과가 보존된 Native/Web PNG의 RGB와 각각 같다. Node26·22의 출력JSON이 같고 모두 정상 종료했다. 제품 화면 검사의 허용 마스크가 아니다.

~~~powershell
node webpj/tools/verify-construction-pixel-observation.mjs
~~~

V4 EXE와 백업의 SHA523b0926…, V5 SHA389251f4…, V6 SHAe664f44e…를 보존했다. V6 전체2,008,432BYTE 원문은 Git 제외 경로 extracted/webpj-render-order-probe/pixel-v6/run1/pixel.tsv에 있으며 SHA-256은5186e3dabb56baac7b6aecaf56573517c5f7c14a9d6401a544141777e991444f 다. 핵심12행·계측 patch·출처/해시는 이 문서의 evidence 폴더에 보존했다. [파일 해시](webpj-e1380-evidence/evidence-hashes.json) · [Native 계측 patch](webpj-e1380-evidence/native-instrumentation.patch).

정기 전체 회귀는 Node26.13.0·Node22.23.5 순차 실행으로 각각6,867/6,867 통과했다. 실패·취소·미실행은 모두0이며 소요 시간은 각각601.122초·821.347초다. [최종 검증값](webpj-e1380-evidence/verification.json). 사용자 일시정지 요청에 따라 이 단위 완료 후 문서·커밋을 마치고 목표를 일시 정지한다. 준비한 후속 HUD 앱 연결 초안은 제품에 적용하지 않는다.

## 일시정지와 재개 지점

후속 HUD 앱 연결은 [미적용 patch](webpj-e1380-evidence/next-hud-app.patch)와 [원본/초안 해시](webpj-e1380-evidence/next-hud-app.json)로 보존했다. 구문 검사와 git apply --check만 통과했으며 제품에 적용하거나 실제 실행 검증하지 않았다. 재개 시 실제 Client 문맥의 앱 생성/World 도킹 분리를 검토·적용하고, State7 공통 정리/Runtime 반납을 연결한 뒤 AI7시점·전 미션 시작을 재대조한다. Console 기한이나 정렬을 직접 보정하지 않는다.

건설1픽셀의 allocator 입력 판단과미승리1-3·1-4·스커미시, 그 밖의 남은 일은 LEFT_JOBS.webpj.md를 따른다. 자동 공격·포획·희생·승리는 사용자 허용1-1~1-5·스커미시에서 실제 아군AI로만 확인하며 나머지 스테이지의 해당 목표는 사용자 확인 범위로 유지한다.

마감 시점 별도 C++ 작업으로 제품 EXE가 재빌드됐다. 계측 당시 기준선 SHA는 위의 c3daa5bfbebfbcb91ab1b95965f9cecb745e17ec1110e9b574080229227249b7이고, 현재 SHA는12abad5844f0ff6ef1ab11c3d99a3c9eb821257cf68bde882ce888cbd4dcadb0이다. 당시 V5/V6 비간섭·보호 결과는 계측 당시 범위이며 현재 빌드를 재검증한 결과가 아니다. GumpEvents/UberGump/TutorialObjective와 assets/lang 번역표 등 별도 작업의 수정·신규 파일은 이 단위에서 수정·스테이징하지 않았다. 재개 시 최신 C++ 변경 판정과 기준선 갱신 여부를 먼저 확인한다.
