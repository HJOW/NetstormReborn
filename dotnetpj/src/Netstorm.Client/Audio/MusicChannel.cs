using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 실제 음악 채널의96BYTE와 부분 읽기·정지 수명을 원문 순서로 처리한다.
namespace Netstorm.Client.Audio;

/// <summary>원문 음악 채널의32비트 필드 오프셋이다.</summary>
public enum MusicField : uint
{
    /// <summary>실제 장치 버퍼의32비트 토큰이다.</summary>
    Buffer = 0,
    /// <summary>현재 장치 버퍼의 전체BYTE 수다.</summary>
    BufferBytes = 4,
    /// <summary>활성1·파일 반복2 비트다.</summary>
    Flags = 8,
    /// <summary>닫힌 뒤에도 실패 경로가 남길 수 있는 실제 파일 토큰이다.</summary>
    File = 0x0c,
    /// <summary>실제 표본BYTE 길이다.</summary>
    Length = 0x10,
    /// <summary>+18부터8BYTE인 원문double 곡 길이다.</summary>
    Duration = 0x18,
    /// <summary>표본 시작에서 읽은BYTE 위치다.</summary>
    ReadOffset = 0x20,
    /// <summary>장치 링 버퍼의 다음 쓰기BYTE 위치다.</summary>
    WriteOffset = 0x24,
    /// <summary>18BYTE WAVEFORMATEX의 시작이다.</summary>
    Format = 0x28,
    /// <summary>파일 안의 표본 시작 절대 위치다.</summary>
    DataOffset = 0x3c,
    /// <summary>원문 중첩 정지의DWORD 깊이다.</summary>
    StopDepth = 0x40,
}

/// <summary>cpppj/src/client/SoundMusic.h의 실제0x60BYTE 저장소다. CLR 잠금과 OS 포인터를 배열에 넣지 않는다.</summary>
public sealed class MusicChannelState
{
    /// <summary>패딩·원문 잠금 공간까지 포함한 실제 채널 배치 크기다.</summary>
    public const int Bytes = 0x60;
    /// <summary>새 채널에서만0으로 시작하며 이후 선택적 필드 쓰기가 이웃을 보존한다.</summary>
    private readonly byte[] _raw = new byte[Bytes];
    /// <summary>현재 같은 원시 저장소를 복사 없이 빌린다.</summary>
    public Memory<byte> Raw() => _raw;
    /// <summary>비정렬도 허용하는 원문DWORD를 리틀엔디언으로 읽는다.</summary>
    public uint Field(MusicField field)
    {
        uint offset = (uint)field;
        if (offset > Bytes - 4) throw new ArgumentOutOfRangeException(nameof(field), "음악 필드의 실제96BYTE 범위 밖 읽기");
        return BinaryPrimitives.ReadUInt32LittleEndian(_raw.AsSpan((int)offset, 4));
    }
    /// <summary>지정DWORD만 쓰고 패딩·다른필드·실제 원문 잠금 공간은 보존한다.</summary>
    public void SetField(MusicField field, uint value)
    {
        uint offset = (uint)field;
        if (offset > Bytes - 4) throw new ArgumentOutOfRangeException(nameof(field), "음악 필드의 실제96BYTE 범위 밖 쓰기");
        BinaryPrimitives.WriteUInt32LittleEndian(_raw.AsSpan((int)offset, 4), value);
    }
    /// <summary>SceneMusic과 같은+18의8BYTE를 읽고 NaN 등 원시double 비트를 정규화하지 않는다.</summary>
    public double Duration() => SceneMusic.ChannelDuration(_raw);
}

/// <summary>같은 열린 파일에서 실제 요청한BYTE 수 또는 실패-1을 반환하는 읽기 경계다.</summary>
public delegate int MusicChannelRead(uint file, Span<byte> target);

/// <summary>원문 파일·버퍼·진단 경계다. 실제 필수 여섯 함수는 생성 때 검사한다.</summary>
public sealed class MusicChannelHooks
{
    /// <summary>현재 실제 버퍼의 음량을 적용하고 HRESULT를 반환한다.</summary>
    public Func<uint, int, int>? SetVolume { get; set; }
    /// <summary>현재 실제 버퍼 재생을 정지한다.</summary>
    public Action<uint>? Stop { get; set; }
    /// <summary>현재 버퍼 COM 참조를 반납하고 signed 반환값을 보존한다.</summary>
    public Func<uint, int>? Release { get; set; }
    /// <summary>실제 파일 토큰의 절대 위치로 이동하고 실패-1을 반환한다.</summary>
    public Func<uint, uint, int>? Seek { get; set; }
    /// <summary>현재 실제 파일에서 요청 구간으로 부분 읽는다.</summary>
    public MusicChannelRead? Read { get; set; }
    /// <summary>실제 파일을 닫는다. 만료 토큰의 안전한 재닫기는 파일 소유자가 보장한다.</summary>
    public Action<uint>? Close { get; set; }
    /// <summary>원문 채널·개행을 포함한 오류 문장을 같은 위치에서 기록한다.</summary>
    public Action<uint, string>? Log { get; set; }
    /// <summary>원문 진단 식과 줄 번호를 보고하고 정상 복귀하면 원문을 계속한다.</summary>
    public Action<string, int>? Report { get; set; }
    /// <summary>실제 재귀 잠금의 진입/이탈 관측이다. 원문 계약상 예외를 던지지 않아야 한다.</summary>
    public Action<bool>? LockEvent { get; set; }
    /// <summary>함수 슬롯은 복사하되 각 callback의 실제 소유자는 그대로 공유한다.</summary>
    public MusicChannelHooks Copy() => (MusicChannelHooks)MemberwiseClone();
}

/// <summary>cpppj/src/client/SoundMusic.cpp의 실제 채널 제어·읽기·되감기·시작 부분 이식이다.
/// Open·버퍼 확보·Fill은 같은 partial의 실제 파일/COM 경계를 사용하고 worker 수명은 별도로 조립한다.</summary>
public sealed partial class MusicChannel
{
    /// <summary>진단 줄 번호를 선택하는 원문 판본이다.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>모든 실제 자식이 공유할 동일96BYTE 채널이다.</summary>
    private readonly MusicChannelState _state;
    /// <summary>생성 시 함수 슬롯을 복사한 실제 파일·버퍼 장치다.</summary>
    private readonly MusicChannelHooks _hooks;
    /// <summary>원문 CRITICAL_SECTION과 같은 재귀 잠금이며 원시96BYTE 밖에 둔다.</summary>
    private readonly object _gate = new();
    /// <summary>같은 실제 채널 저장소를 빌린다. 조회 자체로 초기화하지 않는다.</summary>
    public MusicChannelState State => _state;

    /// <summary>필수 실제 장치를 검사하고 상태와 함께 보관한다. 파일·버퍼·스레드를 열지 않는다.</summary>
    public MusicChannel(OriginalEdition edition, MusicChannelState state, MusicChannelHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.SetVolume is null || hooks.Stop is null || hooks.Release is null || hooks.Seek is null || hooks.Read is null || hooks.Close is null)
            throw new ArgumentException("음악 채널의 필수 실제 장치 누락", nameof(hooks));
        _edition = edition; _state = state; _hooks = hooks.Copy();
    }

    /// <summary>실제 재귀 잠금 한 번과 원문 진입/이탈 관측의 수명을 함께 유지한다.</summary>
    private readonly struct Guard : IDisposable
    {
        /// <summary>같은 실제 채널의 잠금과 장치 소유자다.</summary>
        private readonly MusicChannel _owner;
        /// <summary>실제 잠금을 먼저 획득하고 명시 진입 관측을 호출한다.</summary>
        public Guard(MusicChannel owner)
        {
            _owner = owner;
            Monitor.Enter(owner._gate);
            try { owner._hooks.LockEvent?.Invoke(true); }
            catch { Monitor.Exit(owner._gate); throw; }
        }
        /// <summary>정상·예외 경로 모두 이탈 관측 뒤 실제 잠금 한 번을 반납한다.</summary>
        public void Dispose()
        {
            try { _owner._hooks.LockEvent?.Invoke(false); }
            finally { Monitor.Exit(_owner._gate); }
        }
    }

    /// <summary>원문 오류 채널9와 문장을 개행까지 그대로 전달한다.</summary>
    private void Log(string text) => _hooks.Log?.Invoke(9, text);

    /// <summary>실제 현재 활성 비트만 같은 명령 잠금으로 읽는다. 원문 Guard 관측은 추가하지 않는다.</summary>
    public bool Active() { lock (_gate) return (_state.Field(MusicField.Flags) & 1u) != 0; }

    /// <summary>버퍼가 없어도 같은 잠금을 수행하며 실제 음량 HRESULT 실패만 기록한다.</summary>
    public void SetVolume(int volume)
    {
        using var guard = new Guard(this);
        uint buffer = _state.Field(MusicField.Buffer);
        if (buffer != 0 && _hooks.SetVolume!(buffer, volume) < 0) Log("Failed to set music buffer volume.\n");
    }

    /// <summary>현재 실제 파일을 표본 시작으로 되감고 실패 시 토큰을 남긴 채 닫은 뒤 잠금 밖에서 기록한다.</summary>
    public bool Rewind()
    {
        bool success;
        using (var guard = new Guard(this))
        {
            success = _hooks.Seek!(_state.Field(MusicField.File), _state.Field(MusicField.DataOffset)) != -1;
            if (success) _state.SetField(MusicField.ReadOffset, 0);
            else _hooks.Close!(_state.Field(MusicField.File));
        }
        if (!success) Log("Failed to rewind music file.\n");
        return success;
    }

    /// <summary>요청과 실제 읽기 길이가 다르면 현재 파일을 닫고 토큰·부분 출력은 보존한다.</summary>
    private bool ReadExact(Span<byte> target)
    {
        if (_hooks.Read!(_state.Field(MusicField.File), target) == target.Length) return true;
        _hooks.Close!(_state.Field(MusicField.File));
        return false;
    }

    /// <summary>끝에 도달한 한 번 재생은0으로 채우고 반복은 한 번만 되감는다. 실패한 부분 출력과 선행 위치 쓰기를 보존한다.</summary>
    public int Read(Span<byte> target)
    {
        using var guard = new Guard(this);
        if (target.IsEmpty) return 0;
        uint length = _state.Field(MusicField.Length), offset = _state.Field(MusicField.ReadOffset);
        if (length > int.MaxValue || offset > length) throw new ArgumentOutOfRangeException(nameof(target), "음악 읽기 길이/위치 범위 밖");
        uint remaining = length - offset;
        if ((uint)target.Length < remaining)
        {
            if (!ReadExact(target)) return 0;
            _state.SetField(MusicField.ReadOffset, unchecked(offset + (uint)target.Length));
            return target.Length;
        }
        if (remaining != 0)
        {
            if (!ReadExact(target[..(int)remaining])) return 0;
            _state.SetField(MusicField.ReadOffset, unchecked(offset + remaining));
        }
        if ((_state.Field(MusicField.Flags) & 2u) == 0)
        {
            target[(int)remaining..].Clear();
            return (int)remaining;
        }
        _ = Rewind(); // 원문은 실패 반환을 검사하지 않고 현재 길이 또는 나머지 읽기로 진행한다.
        if ((uint)target.Length == remaining) return (int)remaining;
        Span<byte> rest = target[(int)remaining..];
        if (!ReadExact(rest)) return 0;
        _state.SetField(MusicField.ReadOffset, unchecked(_state.Field(MusicField.ReadOffset) + (uint)rest.Length));
        return target.Length;
    }

    /// <summary>진단을 잠금 전에 실행하고 되감기 정상 복귀 뒤에만 활성/반복과 쓰기 위치를 기록한다.</summary>
    public bool Start(uint loop)
    {
        if (_hooks.Report is { } report)
        {
            bool patch = _edition == OriginalEdition.Patch1078;
            if (_state.Field(MusicField.Length) == 0) report("length != 0", patch ? 0x509 : 0x504);
            if (_state.Field(MusicField.Buffer) == 0) report("buffer != NULL", patch ? 0x50a : 0x505);
        }
        using var guard = new Guard(this);
        if (!Rewind()) return false;
        _state.SetField(MusicField.Flags, loop != 0 ? 3u : 1u);
        _state.SetField(MusicField.WriteOffset, 0);
        return true;
    }

    /// <summary>실제 파일을 닫고 읽기 관련 필드만 지운다. 버퍼·flags·쓰기 위치·+14/+3a 패딩은 보존한다.</summary>
    public void Close()
    {
        using var guard = new Guard(this);
        _hooks.Close!(_state.Field(MusicField.File));
        _state.SetField(MusicField.File, 0);
        _state.SetField(MusicField.Length, 0);
        _state.SetField(MusicField.ReadOffset, 0);
        _state.SetField(MusicField.DataOffset, 0);
        _state.Raw().Span.Slice(0x18, 8).Clear();
        _state.Raw().Span.Slice(0x28, 18).Clear();
    }

    /// <summary>중첩 정지·버퍼 참조 해제·파일 닫기를 원문 재귀 잠금 순서로 실행한다.</summary>
    public void Stop()
    {
        using var outer = new Guard(this);
        using (var inner = new Guard(this))
        {
            if (_state.Field(MusicField.StopDepth) == 0 && _state.Field(MusicField.Buffer) != 0)
                _hooks.Stop!(_state.Field(MusicField.Buffer));
            _state.SetField(MusicField.StopDepth, unchecked(_state.Field(MusicField.StopDepth) + 1u));
            _state.SetField(MusicField.Flags, _state.Field(MusicField.Flags) & ~1u);
        }
        using (var release = new Guard(this))
        {
            uint buffer = _state.Field(MusicField.Buffer);
            if (buffer != 0 && _hooks.Release!(buffer) < 0) Log("Failed to release the sound buffer.\n");
            _state.SetField(MusicField.Buffer, 0);
            _state.SetField(MusicField.BufferBytes, 0);
        }
        Close();
        _state.SetField(MusicField.StopDepth, 0);
    }

    /// <summary>같은 실제 채널의 활성/시작/정지만 빌린다. 이 오버로드의 Open/버퍼는 null이며 실제 파일/버퍼를 주는 오버로드와 구별한다.</summary>
    public MusicPlaybackDevices BindPlaybackDevices() => new() { Active = Active, Start = Start, Stop = Stop };
}
