using System.Runtime.ExceptionServices;
using Netstorm.Core.Original;

// 원문 두 음악 채널 초기화·500ms worker·종료 합류 정책을 실제 명령 소유자에 연결한다.
namespace Netstorm.Client.Audio;

/// <summary>OS 핸들을 원문96BYTE 채널 밖에서 호스트 포인터 폭 그대로 보관한다.</summary>
public sealed class MusicRuntimeState
{
    /// <summary>현재 음악 worker의 실제 커널 경계 핸들이다.</summary>
    public nint Thread { get; set; }
    /// <summary>익명 수동 리셋 종료 이벤트의 실제 커널 경계 핸들이다.</summary>
    public nint Shutdown { get; set; }
    /// <summary>원문 CreateThread가 기록한 마지막DWORD 번호다. 종료 때 임의로0으로 만들지 않는다.</summary>
    public uint ThreadId;
}

/// <summary>callback을 별도 스레드에 보관하며 실제 출력 threadId와 실패0을 반환하는 생성 경계다.</summary>
public delegate nint MusicRuntimeThreadCreate(Func<uint> callback, ref uint threadId);

/// <summary>실제 커널 응답만 공급한다. 생성·대기·신호·중단·닫기·section·log는 예외를 던지지 않는 계약이다.</summary>
public sealed class MusicRuntimeHooks
{
    /// <summary>실제 채널 잠금의 초기화/삭제 위치만 관측하며 CLR 잠금 자체는 채널이 소유한다.</summary>
    public Action<uint, bool>? Section { get; set; }
    /// <summary>익명 수동 리셋·초기 비신호 종료 이벤트를 만든다.</summary>
    public Func<nint>? CreateEvent { get; set; }
    /// <summary>callback을 현재 호출 스레드에서 실행하지 않고 별도 worker에서 실행한다.</summary>
    public MusicRuntimeThreadCreate? CreateThread { get; set; }
    /// <summary>원문 WAIT_OBJECT_0·WAIT_TIMEOUT·WAIT_FAILED의DWORD를 그대로 반환한다.</summary>
    public Func<nint, uint, uint>? Wait { get; set; }
    /// <summary>종료 이벤트를 신호 상태로 만들며 실패를false로 반환한다.</summary>
    public Func<nint, bool>? Signal { get; set; }
    /// <summary>원문 강제 중단 위치의 협조 종료 요청이다. 실제 worker를 강제로 중단하지 않는다.</summary>
    public Action<nint>? Terminate { get; set; }
    /// <summary>스레드는 완전히 합류한 뒤 반납하며 이벤트는 그다음에 닫는다.</summary>
    public Action<nint>? Close { get; set; }
    /// <summary>원문 채널9와 개행을 포함한 완성 문장을 전달한다.</summary>
    public Action<uint, string>? Log { get; set; }
    /// <summary>선택된 원문 assert를 보고한다. 이 호스트 진단의 예외도 만들어진 자원 반납 뒤 전달한다.</summary>
    public Action<string, int>? Report { get; set; }
    /// <summary>함수 슬롯만 복사하고 각 실제 커널 소유자의 수명은 공유한다.</summary>
    public MusicRuntimeHooks Copy() => (MusicRuntimeHooks)MemberwiseClone();
}

/// <summary>cpppj SoundMusicRuntime.cpp의 순수 수명 본체다. 실제 기본 채널·음악 명령 잠금과 커널 경계가 필요하다.</summary>
public sealed class MusicRuntime : IDisposable
{
    /// <summary>원문 worker가 갱신 뒤 기다리는500ms다.</summary>
    public const uint UpdateMilliseconds = 500;
    /// <summary>원문 종료에서 매번 요청하는5000ms다.</summary>
    public const uint ShutdownMilliseconds = 5000;
    /// <summary>원문 worker 반복과 종료 재시도를 허용하는 유일한 대기 결과다.</summary>
    public const uint WaitTimeout = 0x102;
    /// <summary>음악 제어·효과음·장치와 동일한 준비 전역이다.</summary>
    private readonly SoundState _sound;
    /// <summary>현재 실제 커널 자원을 소유하는 동일 핸들 저장소다.</summary>
    private readonly MusicRuntimeState _state;
    /// <summary>실제 음악 제어와 worker가 사용하는 기본 채널이다.</summary>
    private readonly MusicChannel _primary;
    /// <summary>원문 초기화/삭제에만 참여하는 별도 두 번째 채널이다.</summary>
    private readonly MusicChannel _secondary;
    /// <summary>공개 명령과 같은 잠금·Stopping·판본을 보유하는 실제 제어 소유자다.</summary>
    private readonly SoundMusicControl _music;
    /// <summary>실제 FillBuffer의 COM 경계를 함수 슬롯 사본으로 보관한다.</summary>
    private readonly MusicBufferHooks _buffers;
    /// <summary>현재 실제 이벤트/스레드를 소유하는 커널 함수 슬롯의 사본이다.</summary>
    private readonly MusicRuntimeHooks _hooks;
    /// <summary>Initialize와 Shutdown의 중첩 및 서로 다른 호출 스레드의 수명을 직렬화한다.</summary>
    private readonly object _lifecycle = new();
    /// <summary>worker 예외와 주 스레드 진단 조회의 저장소를 보호한다.</summary>
    private readonly object _failureGate = new();
    /// <summary>worker 또는 Dispose에서 잡은 실제 예외다. 정상 종료 뒤에도 다음 초기화까지 보존한다.</summary>
    private Exception? _failure;

    /// <summary>같은 실제 전역·명시 기본 채널과 서로 다른 두 채널을 요구하며 OS 자원은 아직 열지 않는다.</summary>
    public MusicRuntime(SoundState sound, MusicRuntimeState state, MusicChannel primary, MusicChannel secondary,
        SoundMusicControl music, MusicBufferHooks buffers, MusicRuntimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(sound); ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(primary); ArgumentNullException.ThrowIfNull(secondary);
        ArgumentNullException.ThrowIfNull(music); ArgumentNullException.ThrowIfNull(buffers); ArgumentNullException.ThrowIfNull(hooks);
        if (!music.UsesRuntime(sound, primary) || ReferenceEquals(primary, secondary))
            throw new ArgumentException("음악 Runtime의 동일 실제 전역·명시 기본 채널·서로 다른 두 채널 필요");
        if (hooks.CreateEvent is null || hooks.CreateThread is null || hooks.Wait is null || hooks.Signal is null || hooks.Terminate is null || hooks.Close is null)
            throw new ArgumentException("음악 Runtime의 필수 실제 커널 장치 누락", nameof(hooks));
        _sound = sound; _state = state; _primary = primary; _secondary = secondary; _music = music;
        _buffers = buffers.Copy(); _hooks = hooks.Copy();
    }

    /// <summary>동시 worker의 실제 예외를 준비/종료 상태 변경 없이 읽는다.</summary>
    public Exception? WorkerFailure { get { lock (_failureGate) return _failure; } }

    /// <summary>원문 채널9에 같은 개행 문장을 전달한다.</summary>
    private void Log(string text) => _hooks.Log?.Invoke(9, text);

    /// <summary>중복 초기화는 아무것도 쓰지 않는다. 두 채널 선택 초기화 뒤 실제 소리 준비 관문에서만 worker를 만든다.</summary>
    public void Initialize()
    {
        lock (_lifecycle)
        {
            object commands = _music.CommandGate;
            bool held = false;
            try
            {
                Monitor.Enter(commands, ref held);
                if (_sound.MusicInitialized) return;
                lock (_failureGate) _failure = null;
                _primary.ResetForRuntime(0, _hooks.Section);
                _secondary.ResetForRuntime(1, _hooks.Section);
                _music.Stopping = false;
                if (!_sound.Initialized) { Log("Attempting to initialize music with no sound.\n"); return; }
                try
                {
                    _state.Shutdown = _hooks.CreateEvent!();
                    _state.Thread = _hooks.CreateThread!(RunWorker, ref _state.ThreadId);
                    bool patch = _music.Edition == OriginalEdition.Patch1078;
                    if (_state.Thread == 0)
                    {
                        Log("Failed to create music update thread.\n");
                        if (_state.Thread == 0) _hooks.Report?.Invoke("hMusicThread != NULL", patch ? 0x6da : 0x6d5);
                    }
                    else if (_state.Shutdown == 0) _hooks.Report?.Invoke("hMusicShutdown != NULL", patch ? 0x6e0 : 0x6db);
                    _sound.MusicInitialized = true;
                }
                catch
                {
                    _sound.MusicInitialized = true;
                    _music.Stopping = true;
                    Monitor.Exit(commands); held = false;
                    Shutdown();
                    throw;
                }
            }
            finally { if (held) Monitor.Exit(commands); }
        }
    }

    /// <summary>기본 채널 정지 뒤 명령 잠금을 풀고 협조 종료를 기다린다. 종료 예외는 실제 핸들 반납 뒤 다시 전달한다.</summary>
    public void Shutdown()
    {
        lock (_lifecycle)
        {
            ExceptionDispatchInfo? stopFailure = null;
            lock (_music.CommandGate)
            {
                if (!_sound.MusicInitialized) return;
                _music.Stopping = true;
                try { _primary.Stop(); }
                catch (Exception error) { stopFailure = ExceptionDispatchInfo.Capture(error); }
            }
            if (_state.Thread != 0 && _hooks.Wait!(_state.Thread, 0) != 0)
            {
                if (!_hooks.Signal!(_state.Shutdown))
                {
                    Log("Failed to signal music thread shutdown. Terminating thread.\n");
                    _hooks.Terminate!(_state.Thread);
                }
                uint attempts = 0;
                uint result = _hooks.Wait!(_state.Thread, ShutdownMilliseconds);
                // 원문은 TIMEOUT에서만 최대 두 번 중단을 요청하고 다른 실패 대기는 반복하지 않는다.
                while (result == WaitTimeout && attempts < 2)
                {
                    Log("The music update has not died. Attempting to terminate it.\n");
                    _hooks.Terminate!(_state.Thread);
                    attempts++;
                    result = _hooks.Wait!(_state.Thread, ShutdownMilliseconds);
                }
                if (attempts > 1) Log("The music update thread will not die!!\n");
            }
            _hooks.Section?.Invoke(0, false);
            _hooks.Section?.Invoke(1, false);
            if (_state.Thread != 0) { _hooks.Close!(_state.Thread); _state.Thread = 0; }
            if (_state.Shutdown != 0) { _hooks.Close!(_state.Shutdown); _state.Shutdown = 0; }
            lock (_music.CommandGate) _sound.MusicInitialized = false;
            stopFailure?.Throw();
        }
    }

    /// <summary>첫 갱신을 대기보다 먼저 실행한다. 같은 기본 채널만 갱신하고 TIMEOUT일 때만500ms 반복한다.</summary>
    public uint RunWorker()
    {
        try
        {
            nint shutdown = _state.Shutdown;
            if (shutdown == 0) return 0;
            // 준비 전·비활성 자연 반환도 별도 성공을 합성하지 않고 같은 이벤트 대기로 진행한다.
            do { _music.Update(_buffers); }
            while (_hooks.Wait!(shutdown, UpdateMilliseconds) == WaitTimeout);
        }
        catch (Exception error) { lock (_failureGate) _failure = error; }
        return 0;
    }

    /// <summary>CPP 소멸자와 같이 종료를 시도하며 반납 뒤 예외는 밖으로 던지지 않고 진단에 남긴다.</summary>
    public void Dispose()
    {
        try { Shutdown(); }
        catch (Exception error) { lock (_failureGate) _failure = error; }
    }
}
