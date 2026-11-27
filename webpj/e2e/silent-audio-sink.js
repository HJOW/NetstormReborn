// 물리 출력 장치가 없는 GUI 검사에서 실제 Web Audio 엔진의 표준 무출력 sink를 선택한다.
// 게임의 audio 옵션·재생기·난수/시간/상태는 유지하며 실제 청취 검증으로 사용하지 않는다.
async function installSilentAudioSink(target) {
  if (process.env.NETSTORM_E2E_SILENT_SINK !== '1') return;
  await target.addInitScript(() => {
    // 원래 네이티브 생성자와 검사 중 생성한 실제 AudioContext의 관측용 목록이다.
    const NativeContext = window.AudioContext ?? window.webkitAudioContext, contexts = [];
    if (!NativeContext || !('setSinkId' in NativeContext.prototype)) throw new Error('검사 브라우저의 표준 무출력 AudioContext 지원 없음');
    window.netstormAudioTestContexts = contexts;
    // 네이티브 AudioContext를 상속하고 출력 목적지만 none으로 지정한다. 버퍼·그래프·오디오 시계는 네이티브 구현 그대로다.
    class SilentAudioContext extends NativeContext {
      // 원래 모든 옵션을 보존하고 표준 AudioSinkOptions만 명시한다.
      constructor(options = {}) {
        super({ ...options, sinkId: { type: 'none' } });
        if (this.sinkId?.type !== 'none') throw new Error('검사 AudioContext의 무출력 sink 선택 실패');
        contexts.push(this);
      }
    }
    window.AudioContext = SilentAudioContext;
    if (window.webkitAudioContext) window.webkitAudioContext = SilentAudioContext;
  });
}
module.exports = { installSilentAudioSink };
