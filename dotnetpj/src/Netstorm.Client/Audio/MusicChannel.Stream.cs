// 같은 실제 음악 채널의 링 버퍼를 두 직접 쓰기 구간으로 갱신한다.
namespace Netstorm.Client.Audio;

/// <summary>실제 DirectSound Lock에서 빌린 두 표본 구간이다. 같은 Unlock까지만 유효하며 힙에 보관하지 않는다.</summary>
public ref struct MusicBufferRegions
{
    /// <summary>내부 쓰기 위치부터 링 끝까지의 첫 직접 쓰기 구간이다.</summary>
    public Span<byte> First { get; set; }
    /// <summary>링 시작부터 이어지는 두 번째 직접 쓰기 구간이다.</summary>
    public Span<byte> Second { get; set; }
    /// <summary>같은 실제 Lock이 반환한 두 구간을 복사 없이 빌린다.</summary>
    public MusicBufferRegions(Span<byte> first, Span<byte> second) { First = first; Second = second; }
}

/// <summary>상태 DWORD 출력과 실제 HRESULT를 별도로 보존하는 COM 경계다.</summary>
public delegate int MusicBufferStatus(uint buffer, out uint status);
/// <summary>재생/쓰기 커서 두 출력과 실제 HRESULT를 별도로 보존하는 COM 경계다.</summary>
public delegate int MusicBufferCursor(uint buffer, out uint play, out uint write);
/// <summary>성공했을 때만 같은 실제 두 구간을 갱신하고 HRESULT를 반환하는 Lock 경계다.</summary>
public delegate int MusicBufferLock(uint buffer, uint offset, uint count, ref MusicBufferRegions regions);
/// <summary>같은 실제 Lock의 두 포인터·크기를 그대로 반납하는 Unlock 경계다. 예외 정리 중 예외를 던지지 않아야 한다.</summary>
public delegate int MusicBufferUnlock(uint buffer, MusicBufferRegions regions);

/// <summary>실제 버퍼 생성에 이어 원문 갱신이 선택하는 COM 함수 슬롯을 추가한다.</summary>
public sealed partial class MusicBufferHooks
{
    /// <summary>현재 재생/손실/반복 비트와 HRESULT를 읽는다.</summary>
    public MusicBufferStatus? Status { get; set; }
    /// <summary>현재 실제 버퍼의 손실을 복구한다.</summary>
    public Func<uint, int>? Restore { get; set; }
    /// <summary>현재 실제 재생/쓰기 커서 두 DWORD와 HRESULT를 읽는다.</summary>
    public MusicBufferCursor? Cursor { get; set; }
    /// <summary>요청한 링 구간을 잠그고 같은 실제 메모리 두 구간을 빌린다.</summary>
    public MusicBufferLock? Lock { get; set; }
    /// <summary>빌린 두 실제 구간을 같은 COM 버퍼에 반납한다.</summary>
    public MusicBufferUnlock? Unlock { get; set; }
    /// <summary>파일 반복과 독립인 장치 링 버퍼의 반복 재생을 실행한다.</summary>
    public Func<uint, uint, int>? Play { get; set; }
}

/// <summary>cpppj SoundMusic.cpp의 실제 FillBuffer를 기존 읽기·정지와 같은96BYTE/잠금에 연결한다.</summary>
public sealed partial class MusicChannel
{
    /// <summary>한 COM 오류를 기록한 뒤 원문 중첩 정지를 실행한다. 빈 문장은 짧은 표본 읽기의 별도 기록을 만들지 않는다.</summary>
    private bool StreamFailure(string text)
    {
        if (text.Length != 0) Log(text);
        Stop();
        return false;
    }

    /// <summary>상태/복구·음량·커서·두 실제 Lock 구간 읽기·Unlock·첫 Play를 원문 순서로 실행한다.</summary>
    public bool FillBuffer(MusicBufferHooks? hooks, SoundState sound)
    {
        ArgumentNullException.ThrowIfNull(sound);
        using var guard = new Guard(this);
        if ((_state.Field(MusicField.Flags) & 1u) == 0) return false;
        if (hooks?.Status is null || hooks.Restore is null || hooks.Cursor is null || hooks.Lock is null || hooks.Unlock is null || hooks.Play is null)
            throw new ArgumentException("음악 버퍼 갱신의 실제 장치 누락", nameof(hooks));
        uint buffer = _state.Field(MusicField.Buffer);
        if (buffer == 0) throw new InvalidOperationException("활성 음악 버퍼 누락");
        if (hooks.Status(buffer, out uint status) < 0) return StreamFailure("Failed to get music buffer status.\n");
        if ((status & 2u) != 0 && hooks.Restore(buffer) < 0) return StreamFailure("Failed to restore music buffer.\n");
        if (_hooks.SetVolume!(buffer, sound.MusicVolume) < 0) Log("Failed to set music buffer volume in fillBuffer().\n");
        uint bytes = _state.Field(MusicField.BufferBytes), count;
        if ((status & 5u) == 0)
        {
            _state.SetField(MusicField.WriteOffset, 0);
            count = bytes;
        }
        else
        {
            if (hooks.Cursor(buffer, out uint play, out _) < 0) return StreamFailure("Failed to get current music buffer position.\n");
            uint offset = _state.Field(MusicField.WriteOffset);
            if (play >= bytes || offset >= bytes) throw new ArgumentOutOfRangeException(nameof(hooks), "음악 버퍼 커서 범위 밖");
            count = play < offset ? unchecked(bytes - offset + play) : play - offset;
        }
        if (count != 0)
        {
            if (bytes > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(hooks), "음악 버퍼 크기 범위 밖");
            MusicBufferRegions regions = default;
            if (hooks.Lock(buffer, _state.Field(MusicField.WriteOffset), count, ref regions) < 0)
                return StreamFailure("Failed to lock music buffer.\n");
            bool shortRead = false;
            try
            {
                if ((uint)regions.First.Length > count || (uint)regions.Second.Length != count - (uint)regions.First.Length)
                    throw new ArgumentOutOfRangeException(nameof(hooks), "음악 버퍼 잠금 구간 크기 불일치");
                int first = Read(regions.First);
                if (first == regions.First.Length)
                {
                    _state.SetField(MusicField.WriteOffset, unchecked(_state.Field(MusicField.WriteOffset) + (uint)first));
                    if ((uint)regions.First.Length != count)
                    {
                        int second = Read(regions.Second);
                        if (second != regions.Second.Length) shortRead = true;
                        else _state.SetField(MusicField.WriteOffset, unchecked(_state.Field(MusicField.WriteOffset) + (uint)second));
                    }
                }
                else shortRead = true;
            }
            catch
            {
                _ = hooks.Unlock(buffer, regions);
                throw;
            }
            _state.SetField(MusicField.WriteOffset, _state.Field(MusicField.WriteOffset) % bytes);
            if (hooks.Unlock(buffer, regions) < 0) return StreamFailure("Failed to unlock music buffer.\n");
            if (shortRead) return StreamFailure("");
        }
        if ((status & 5u) == 0 && hooks.Play(buffer, 1) < 0) return StreamFailure("Failed to play music buffer.\n");
        return true;
    }
}
