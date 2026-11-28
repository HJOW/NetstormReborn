using System.Buffers;
using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Client.Audio;

// 음악 표본을 전부 적재하지 않고 실제 Winmm 열린 파일의 위치와 수명을 유지한다.
namespace Netstorm.Client.Host;

/// <summary>cpppj SoundMusicFile.cpp의 실제 파일 저장소다. 장치·worker를 만들지 않고 채널보다 오래 파일 핸들을 소유한다.</summary>
[SupportedOSPlatform("windows")]
public sealed class WindowsMusicFileStore : IDisposable
{
    /// <summary>원문 MMCKINFO의 DWORD 다섯 개다.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct Chunk
    {
        /// <summary>현재 청크 FourCC다.</summary>
        public uint Id;
        /// <summary>Winmm이 현재 파일/RIFF에 맞춰 돌려준 청크 크기다.</summary>
        public uint Size;
        /// <summary>RIFF의 WAVE 식별자다.</summary>
        public uint Type;
        /// <summary>같은 실제 파일의 청크 데이터 절대 위치다.</summary>
        public uint Offset;
        /// <summary>Winmm이 관리하는 청크 플래그다.</summary>
        public uint Flags;
    }

    /// <summary>원문 MMIO_ALLOCBUF|MMIO_READ다. 파일은 읽기 전용으로 연다.</summary>
    private const uint ReadWithBuffer = 0x10000;
    /// <summary>MMIO_FINDRIFF로 현재 WAVE RIFF를 찾는다.</summary>
    private const uint FindRiff = 0x20;
    /// <summary>MMIO_FINDCHUNK로 현재 RIFF의 fmt/data를 찾는다.</summary>
    private const uint FindChunk = 0x10;
    /// <summary>파일 표·다음 토큰·각 Winmm 위치를 한 호출 동안 보호하는 재귀 Monitor다.</summary>
    private readonly object _gate = new();
    /// <summary>실제 HMMIO를 호스트 포인터 폭 그대로 보유하는 비영 토큰 표다.</summary>
    private readonly Dictionary<uint, nint> _files = [];
    /// <summary>반납한 토큰을 재사용하지 않는 다음 DWORD다. 감긴0은 소진을 뜻한다.</summary>
    private uint _nextToken = 1;
    /// <summary>짧은 비PCM fmt에서 원문 문장과 줄을 전달하는 선택 진단 장치다.</summary>
    private readonly Action<string, int>? _report;
    /// <summary>종료 뒤 새 파일을 열지 못하게 하는 소유자 수명 표식이다.</summary>
    private bool _disposed;

    /// <summary>유니코드 경로의 읽기 전용 Winmm 파일을 연다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioOpenW", CharSet = CharSet.Unicode)]
    private static extern nint NativeOpen(string path, nint information, uint flags);
    /// <summary>같은 실제 파일 핸들을 반납한다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioClose")]
    private static extern uint NativeClose(nint file, uint flags);
    /// <summary>부모 없는 현재 RIFF를 찾는다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioDescend")]
    private static extern uint Descend(nint file, ref Chunk chunk, nint parent, uint flags);
    /// <summary>현재 RIFF 안에서 요청 청크를 찾는다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioDescend")]
    private static extern uint Descend(nint file, ref Chunk chunk, ref Chunk parent, uint flags);
    /// <summary>현재 청크 끝으로 이동한다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioAscend")]
    private static extern uint Ascend(nint file, ref Chunk chunk, uint flags);
    /// <summary>현재 위치에서 요청만큼 읽으며 실제 부분 출력과 signed 길이를 돌려준다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioRead")]
    private static extern int NativeRead(nint file, [In, Out] byte[] target, int count);
    /// <summary>현재 실제 파일의 signed32 위치를 이동하거나 조회한다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioSeek")]
    private static extern int NativeSeek(nint file, int offset, int origin);

    /// <summary>진단만 보관하고 파일·장치·스레드를 만들지 않는다.</summary>
    public WindowsMusicFileStore(Action<string, int>? report = null)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 음악 파일 장치는 Windows 전용이다");
        _report = report;
    }

    /// <summary>같은 입출력 헤더의 원문 부분 쓰기를 보존하며 파일은 성공했을 때만 열린 채로 인계한다.</summary>
    public bool Open(string path, MusicFileHeader header)
    {
        ArgumentNullException.ThrowIfNull(path); ArgumentNullException.ThrowIfNull(header);
        lock (_gate)
        {
            ObjectDisposedException.ThrowIf(_disposed, this);
            nint handle = NativeOpen(path, 0, ReadWithBuffer);
            header.File = 0;
            if (handle == 0) return false;
            if (_nextToken == 0) { NativeClose(handle, 0); throw new OverflowException("음악 파일 토큰이 소진됐습니다"); }
            uint token = _nextToken;
            _nextToken = unchecked(_nextToken + 1);
            header.File = token;
            try { _files.Add(token, handle); }
            catch { NativeClose(handle, 0); throw; }
            bool keep = false;
            try
            {
                var riff = new Chunk { Type = 0x45564157 };
                if (Descend(handle, ref riff, 0, FindRiff) != 0) return false;
                var chunk = new Chunk { Id = 0x20746d66 };
                if (Descend(handle, ref chunk, ref riff, FindChunk) != 0 || chunk.Size < 16) return false;
                int size = (int)Math.Min(chunk.Size, 18);
                if (NativeRead(handle, header.Format, size) != size) return false;
                // 진단 호출 중 입력 헤더가 바뀌어도 원문 지역 WAVEFORMATEX의 값을 그대로 사용한다.
                Span<byte> format = stackalloc byte[18];
                header.Format.CopyTo(format);
                ushort tag = BinaryPrimitives.ReadUInt16LittleEndian(format);
                if (size < 18)
                {
                    if (tag != 1) _report?.Invoke("wfFormat.wFormatTag == WAVE_FORMAT_PCM", 0xeb);
                    uint denominator = unchecked((uint)BinaryPrimitives.ReadUInt16LittleEndian(format[2..]) * BinaryPrimitives.ReadUInt32LittleEndian(format[4..]));
                    if (denominator == 0) return false;
                    uint bits = unchecked(BinaryPrimitives.ReadUInt32LittleEndian(format[8..]) << 3) / denominator;
                    BinaryPrimitives.WriteUInt16LittleEndian(format[16..], 0);
                    BinaryPrimitives.WriteUInt16LittleEndian(format[14..], unchecked((ushort)bits));
                    format.CopyTo(header.Format);
                }
                if (BinaryPrimitives.ReadUInt16LittleEndian(format[2..]) == 0 || BinaryPrimitives.ReadUInt32LittleEndian(format[4..]) == 0
                    || BinaryPrimitives.ReadUInt16LittleEndian(format[14..]) == 0 || (tag != 1 && BinaryPrimitives.ReadUInt16LittleEndian(format[16..]) != 0)) return false;
                if (Ascend(handle, ref chunk, 0) != 0) return false;
                chunk = new Chunk { Id = 0x61746164 };
                if (Descend(handle, ref chunk, ref riff, FindChunk) != 0 || chunk.Size == 0 || chunk.Size > int.MaxValue || chunk.Offset > int.MaxValue) return false;
                header.Length = chunk.Size;
                header.DataOffset = chunk.Offset;
                keep = true;
                return true;
            }
            finally { if (!keep) Close(token); }
        }
    }

    /// <summary>같은 현재 파일만 signed32 절대 위치로 이동한다. 닫힌 토큰과 범위 밖 위치는-1이다.</summary>
    public int Seek(uint token, uint absolute)
    {
        lock (_gate) return absolute <= int.MaxValue && _files.TryGetValue(token, out nint file) ? NativeSeek(file, (int)absolute, 0) : -1;
    }

    /// <summary>현재 파일에서 요청 Span 크기만 읽는다. 부분 출력·실제 읽은 길이를 보존하고 표본 전체를 적재하지 않는다.</summary>
    public int Read(uint token, Span<byte> target)
    {
        lock (_gate)
        {
            if (!_files.TryGetValue(token, out nint file)) return -1;
            if (target.IsEmpty) return 0;
            byte[] buffer = ArrayPool<byte>.Shared.Rent(target.Length);
            try
            {
                // Winmm의 짧은 읽기/실패가 쓰지 않은 목적지 바이트를 그대로 유지한다.
                target.CopyTo(buffer);
                int count = NativeRead(file, buffer, target.Length);
                buffer.AsSpan(0, target.Length).CopyTo(target);
                return count;
            }
            finally { ArrayPool<byte>.Shared.Return(buffer); }
        }
    }

    /// <summary>표에서 먼저 제거한 자기 핸들만 닫는다. 0·만료 토큰의 재닫기는 다른 파일에 영향을 주지 않는다.</summary>
    public void Close(uint token)
    {
        lock (_gate) if (_files.Remove(token, out nint file)) NativeClose(file, 0);
    }

    /// <summary>같은 열린 파일의 현재 절대 위치 또는 닫힌 토큰의-1을 반환한다.</summary>
    public int Position(uint token)
    {
        lock (_gate) return _files.TryGetValue(token, out nint file) ? NativeSeek(file, 0, 1) : -1;
    }

    /// <summary>기존 장치/로그/잠금 함수를 값 복사하고 파일IO 세 함수만 같은 저장소에 연결한다.</summary>
    public MusicChannelHooks FileHooks(MusicChannelHooks? hooks = null)
    {
        lock (_gate)
        {
            ObjectDisposedException.ThrowIf(_disposed, this);
            MusicChannelHooks result = hooks?.Copy() ?? new();
            result.Seek = Seek; result.Read = Read; result.Close = Close;
            return result;
        }
    }

    /// <summary>음악의 한 검색 문자열을 효과음 resolver의 기본 루트에서 한 번 조회하고 유니코드 실제 경로를 연다.</summary>
    public MusicOpenHooks OpenHooks(Func<string, bool, string?> resolver, Action<string>? fallback = null)
    {
        ArgumentNullException.ThrowIfNull(resolver);
        lock (_gate)
        {
            ObjectDisposedException.ThrowIf(_disposed, this);
            return new() { Find = pattern => resolver(pattern, false), Open = Open, Fallback = fallback };
        }
    }

    /// <summary>worker 종료/합류 뒤 남은 모든 실제 파일을 닫고 새 Open을 막는다. 반복 종료는 안전하다.</summary>
    public void Dispose()
    {
        lock (_gate)
        {
            if (_disposed) return;
            _disposed = true;
            // 각 실제 핸들은 표의 자기 토큰으로 한 번만 반납한다.
            foreach (nint file in _files.Values) NativeClose(file, 0);
            _files.Clear();
        }
    }
}
