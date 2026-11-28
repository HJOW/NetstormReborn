using System.Buffers.Binary;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Netstorm.Client.Audio;

// 효과음과 동일한 실제 DirectSound 장치/버퍼 토큰에 음악 생성·갱신 COM을 연결한다.
namespace Netstorm.Client.Host;

/// <summary>cpppj SoundDevice::BindMusicBuffers·MusicBuffers의 실제 COM 경계다. worker는 만들지 않는다.</summary>
public sealed partial class WindowsSoundDevice
{
    /// <summary>Windows SDK DSERR_NODRIVER=MAKE_DSHRESULT(120)이다.</summary>
    private const int MusicNoDriver = unchecked((int)0x88780078);
    /// <summary>Windows SDK DSERR_BADFORMAT=MAKE_DSHRESULT(100)이다.</summary>
    private const int MusicBadFormat = unchecked((int)0x88780064);
    /// <summary>현재 재생/쓰기 위치 두 DWORD를 읽는 실제 IDirectSoundBuffer ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int ReadCursors(nint self, out uint play, out uint write);

    /// <summary>파일 IO/로그/잠금 관측의 사본을 유지하고 같은 실제 버퍼 음량·정지·개별 Release만 붙인다.</summary>
    public MusicChannelHooks BindMusicBuffers(MusicChannelHooks files)
    {
        ArgumentNullException.ThrowIfNull(files);
        MusicChannelHooks result = files.Copy();
        result.SetVolume = (token, volume) => { lock (_gate) { nint buffer = Get(token); return Method<WriteLong>(buffer, 15)(buffer, volume); } };
        result.Stop = token => { lock (_gate) { nint buffer = Get(token); _ = Method<Simple>(buffer, 18)(buffer); } };
        result.Release = token =>
        {
            lock (_gate)
            {
                nint buffer = Get(token);
                int released = unchecked((int)Method<Release>(buffer, 2)(buffer));
                _buffers.Remove(token);
                return released;
            }
        };
        result.Report ??= _report;
        return result;
    }

    /// <summary>생성자/경계 구성만으로 DLL·장치·worker를 열지 않고 실제 COM 함수만 빌린다.</summary>
    public MusicBufferHooks MusicBuffers() => new()
    {
        Create = CreateMusicBuffer,
        Status = (uint token, out uint status) => { lock (_gate) { nint buffer = Get(token); return Method<ReadDword>(buffer, 9)(buffer, out status); } },
        Restore = token => { lock (_gate) { nint buffer = Get(token); return Method<Simple>(buffer, 20)(buffer); } },
        Cursor = (uint token, out uint play, out uint write) => { lock (_gate) { nint buffer = Get(token); return Method<ReadCursors>(buffer, 4)(buffer, out play, out write); } },
        Lock = LockMusicBuffer,
        Unlock = UnlockMusicBuffer,
        Play = (token, flags) => { lock (_gate) { nint buffer = Get(token); return Method<PlayBuffer>(buffer, 12)(buffer, 0, 0, flags); } },
    };

    /// <summary>PCM cbSize만 지역 사본에서 보정하고 같은 장치의 실제 음악 버퍼를 발급한다. 실패 HRESULT는 기존 출력 토큰을 유지한다.</summary>
    private int CreateMusicBuffer(ReadOnlySpan<byte> format, uint flags, uint bytes, ref uint token)
    {
        lock (_gate)
        {
            if (_device == 0) return MusicNoDriver;
            if (format.Length != 18) throw new ArgumentException("음악 형식은 WAVEFORMATEX18BYTE여야 합니다", nameof(format));
            byte[] copy = format.ToArray();
            if (BinaryPrimitives.ReadUInt16LittleEndian(copy) == 1) BinaryPrimitives.WriteUInt16LittleEndian(copy.AsSpan(16), 0);
            else if (BinaryPrimitives.ReadUInt16LittleEndian(copy.AsSpan(16)) != 0) return MusicBadFormat;
            nint raw = Marshal.AllocHGlobal(18);
            try
            {
                Marshal.Copy(copy, 0, raw, 18);
                var description = new BufferDescription { Size = (uint)Marshal.SizeOf<BufferDescription>(), Flags = flags, Bytes = bytes, Format = raw };
                int result = Method<CreateBuffer>(_device, 3)(_device, ref description, out nint buffer, 0);
                if (result >= 0) token = Add(buffer);
                return result;
            }
            finally { Marshal.FreeHGlobal(raw); }
        }
    }

    /// <summary>실제 COM이 반환한 포인터를 복사 없는 Span으로 빌린다. 같은 Unlock 전에는 Runtime/장치를 종료하면 안 된다.</summary>
    private unsafe int LockMusicBuffer(uint token, uint offset, uint count, ref MusicBufferRegions regions)
    {
        lock (_gate)
        {
            nint buffer = Get(token);
            int result = Method<LockBuffer>(buffer, 11)(buffer, offset, count, out nint first, out uint firstSize, out nint second, out uint secondSize, 0);
            if (result >= 0)
            {
                try
                {
                    if (firstSize > int.MaxValue || secondSize > int.MaxValue)
                        throw new ArgumentOutOfRangeException(nameof(count), "음악 COM 구간의 Span 범위 밖 크기");
                    regions = new(new Span<byte>((void*)first, (int)firstSize), new Span<byte>((void*)second, (int)secondSize));
                }
                catch
                {
                    _ = Method<UnlockBuffer>(buffer, 19)(buffer, first, firstSize, second, secondSize);
                    throw;
                }
            }
            return result;
        }
    }

    /// <summary>길이0이어도 원래 Span 시작 주소를 유지하고 같은 실제 포인터·크기를 Unlock에 돌려준다.</summary>
    private unsafe int UnlockMusicBuffer(uint token, MusicBufferRegions regions)
    {
        lock (_gate)
        {
            nint buffer = Get(token);
            nint first = (nint)Unsafe.AsPointer(ref MemoryMarshal.GetReference(regions.First));
            nint second = (nint)Unsafe.AsPointer(ref MemoryMarshal.GetReference(regions.Second));
            return Method<UnlockBuffer>(buffer, 19)(buffer, first, (uint)regions.First.Length, second, (uint)regions.Second.Length);
        }
    }
}
