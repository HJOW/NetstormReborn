# 실제 전투 화면 차이와 State7 HUD 생성 시점

- 단위: WB04-E1373, 2026-11-26, HJOW-Athlon.
- 이 단위는 실제 관측·원인 분류·재현 도구를 완료한다. 제품 HUD 수명 복원과8000프레임1픽셀의 직접 원인은 후속 작업이다. 7단계는6/7을 유지한다.

## 확인된 화면 표현

현행 C++ 제품1370의 독립 관측과 E1370 웹 실제 실행의 차이를 원본 SHP·소유자색표·GIFCLOUD 팔레트로 다시 계산했다. 아래 두 시점은 실제 PNG 전체 차이 좌표 집합과 계산한 겹침 좌표 집합이 완전히 같다. 각 픽셀의 Native·웹 RGB도 해당 최상위 프레임 색과 모두 같다.

| 시점 | 위치 | Native 최상위 / 웹 최상위 | 실제 차이 |
| --- | --- | --- | ---: |
| 7000 | 1008,494 | SID1116 frame0 / SID1129 frame3 | 31 |
| 12000 | 384,307 | SID1730 frame1 / SID842 frame2 | 42 |
| 12000 | 1008,494 | SID1116 frame0 / SID1129 frame3 | 31 |
| 12000 | 816,769 | SID1160 frame1 / SID1217 frame0 | 화면 안26 |

12000 합계는99픽셀이며 마지막 쌍의 나머지15픽셀은 화면 아래다. 모두 TYPE155 다리 연결부다. 이 시점들의 살아 있는 Shrapnel TYPE110 더미/TYPE16 슬롯은0개여서 살아 있는 파편이 직접 이 차이를 그렸다는 설명과는 맞지 않는다.

8000의1픽셀(947,689)은 Native RGB(0,0,0), 웹(45,25,19), 원본 팔레트11이다. 이 위치는 SID1292 TYPE105 마나 볼트의 실제 그림자 마스크 안이지만, 그 그림자의 최종 대상색 읽기·이전 갱신 잔류 중 무엇이 직접 원인인지는 아직 확정하지 않았다.

[실제 PNG·계산값·원문 해시](webpj-e1373-evidence/provenance.json)와 [픽셀 증거](webpj-e1373-evidence/pixel-proof.json)를 보존했다. E1372 소스에서 Guide 뒤 Ready 재검사를 넣은 별도 조사 실행도7시점 덤프 차이0과 기존31·1·99픽셀 차이를 그대로 냈다. 그 실행의 세 PNG는 앞선 E1370 관측 PNG와 SHA-256을 포함해 바이트가 같다. 이 재검사 경로는 수정으로 채택하지 않았다.

## 실제 갱신·생성 시점

독립 C++ 관측 사본에서 정렬 전 항목과 전체 무효화 요청의 호출 스택을 읽었다. Native는6984프레임에 전체 갱신하고 웹은6985프레임이다. 같은 clip의 객체·정렬키·수집 순서는 같았고 kind1 그룹 헤더의 사용하지 않는 tail 잔재만 달랐다. kind1 분배는 헤더 tail 대신 실제 멤버 항목을 읽으므로 이 잔재를 원인으로 삼지 않았다.

실제 전체 갱신 요청자는 TextEntryGump::Idle → Redraw → RawButtonControls의 공통 changed → Renderer::InvalidateAll이다. 숨겨진 Console도 readOnly/dead가 아니면 점멸 Idle을 수행한다. 두 구현의 반초 주기 계산은 같다.

| 최초 미션 입력줄 관측 | wall | blinkAt | 이후 상태 |
| --- | ---: | ---: | --- |
| C++ frame4 | 9.2 | 9.7 | frame5에도 같은 Chat555 / Console559 유지 |
| 웹 frame5 | 9.225 | 9.725 | Chat15 / Console19 유지 |
| 6000 | 양쪽159.1 | C++159.2 / 웹159.225 | 위상299로 같고 기한25ms 차이 |
| 7000 | 양쪽184.1 | C++184.2 / 웹184.225 | 위상349로 같고 기한25ms 차이 |

토큰 자체는 UI 저장소마다 다르다. [Native 원문](webpj-e1373-evidence/native-ui.tsv) · [웹 원문](webpj-e1373-evidence/web-ui.json)

## 생성 경로의 차이

C++의 State7은 CleanupNativeStateWorld(1)에서 기존 Chat(command9)을 유지 타입11 이외의 Gump와 함께 지우고, 자식 우선 반납 뒤 ReplaceHudForState → UberRootGump::Replace → createChat을 실행한다. 새 Console은 TextEntryGump::Construct에서 현재 wall+0.5로 첫 기한을 잡는다. Ready 뒤 InstallHud는 현재 Uber가 있으면 재생성하지 않고 Reflow·팔레트·월드 연결을 수행한다.

이 경로는 정적 소스뿐 아니라 초기 실제 호출 스택에서도 CleanupStateWorld → RawUberRootControls::Replace → UberRootGump::Construct → ChatWindow::Create/Construct → TextEntryGump::Construct를 확인했다. [실제 원문 스택](webpj-e1373-evidence/native-call-stacks.tsv) · [같은 관측 사본 MAP으로 해석한 생성 스택](webpj-e1373-evidence/native-constructor-stacks.json). 스택의 afterFrame은 마지막 Renderer 수집 번호이므로 생성 당시의 프레임 번호로 읽지 않는다. 생성 시점 표는 위의 실제 UI 스냅샷이다.

웹 _runCampaignState7은 전체 정리 주석 아래 _clearPlayerSpForState만 실행한다. 공통 cleanupClientStateWorld 부모는 제품에 미연결이며 HUD·Chat은 Ready 뒤에 처음 생성한다. ClientUberHud.install은 교체 생성과 최초 mode3 도킹을 묶고, 생산 조립기는 즉시 Session을 요구한다. Ready 확인을 Guide 뒤에 추가한 실험이 효과 없었던 것도 확인했다.

근거: [Native State7](../cpppj/src/client/UberGump.cpp), [HUD 공장·교체·Ready 연결](../cpppj/src/client/UberHud.inc), [공통 정리](../cpppj/src/o/RawStateWorldCleanup.cpp), [Console 기한](../cpppj/src/client/TextEntryGump.cpp), [웹 State7](../webpj/www/netstorm.js), [웹 공통 정리](../webpj/www/core/client-state-world-cleanup.js), [웹 Uber 설치](../webpj/www/ui/client-uber-hud.js), [Session 의존 생산 조립](../webpj/www/ui/client-production-assembly.js).

## 관측 사본과 재현

원래 C++ 제품 SHA-256은 c3daa5bfbebfbcb91ab1b95965f9cecb745e17ec1110e9b574080229227249b7이며 그대로 유지했다. 추적 EXE는 Git 제외 extracted/webpj-render-order-probe/Netstorm.exe에만 따로 링크했다. [정확한 관측 변경점](webpj-e1373-evidence/native-instrumentation.patch)과 [실제 실행 명령·완료 시각·추적 EXE 해시](webpj-e1373-evidence/native-source.json)를 보존했다. 관측 사본의1000·6000·7000 화면은 원래 제품과0픽셀, 상태는 주소 잔재 Player와 소유자0 SP를 제외하고0차이다([비간섭 검증](webpj-e1373-evidence/noninterference.log)). 실제 게임·AI·SP1~8 값을 수정하지 않았다.

REA Frida 제공자는 preflight에서 로컬 바인딩 누락 capability_unavailable을 반환해 세션이 만들어지지 않았다. 설정·제품 파일을 바꾸지 않고 별도 읽기 전용 관측 사본을 사용했다. 원본/CD와 제품 C++ 소스·EXE는 수정하지 않았다.

보존된 관측의 자산 대조는 저장소 루트에서 node webpj/tools/verify-ai-overlap-observation.mjs다. 다른 위치의 같은 기록은 폴더를 마지막 인자로 준다. Node26.13.0과22.23.5에서 같은 JSON과 성공 종료를 확인했다. 이 도구는 보존된 실제 차이의 설명을 검증하며 제품 e2e의 픽셀 허용값이 아니다. 제품의 정렬 동률값·마스크·화면 성공 조건은 바꾸지 않았다.

## 이어 할 단위

1. E1374: HUD 공장을 Client 장기 저장소에 연결하고 교체 생성과 Ready 뒤 월드 연결을 분리한다. Session이 아직 없어도 원래 생성 순서를 수행할 수 있게 한다.
2. E1375: State7의 실제 공통 정리·Uber 교체 부모를 연결한다. Tutorial의 HUD 등록 → Guide init → Start/AI와 RawForm SID 순서를 유지한다.
3. E1376 이후: 실제 AI7시점과 전 미션 시작 상태를 다시 대조한다. 화면7곳0픽셀을 확인하기 전에는 전투 표시 전체 완료로 세지 않는다. 마나 볼트 그림자1픽셀도 직접 출력 경로로 확인한다.

제품 www 소스는 이 단위에서 변경하지 않았다. 전체 회귀는 직전 E1372의 두 버전 각6,860/6,860 통과를 유지하며 이 단위의 재현 도구를 두 버전에서 별도로 실행했다.

## E1380 후속 관측

8000프레임의1픽셀은 [건설 전용 heap 인접DWORD와 실제 마지막 작성자](webpj-construction-pixel-memory-20261127.md)로 확정했다. 기존31/99픽셀의 State7/Chat 갱신 위상 원인과는 별개다. 당시 정적 후보832는 실제 전체 캡처 인덱스1978로 정정했다. 제품값은 변경하지 않았고 종류별 명시 메모리 입력 판단은 남아 있다.
