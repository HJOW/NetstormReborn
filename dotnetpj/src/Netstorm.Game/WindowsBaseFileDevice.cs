using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // Windows10/11의 실제 CRT FILE을 Source 본체에 제공한다.

/// <summary>ClientColorMapFile의 FILE/키 소유자이며 실제 CRT 부분 반환과 long32를 유지한다.</summary>
public sealed class WindowsBaseFileDevice : IRawBaseFileDevice
{
    /// <summary>현재 실제 FILE/키와 부분 파일 필드다.</summary>
    public RawBaseFileState State { get; } = new();
    /// <summary>같은 FILE과 키를 사용하는 실제 플랫폼 장치다.</summary>
    public RawBaseFileIOHooks IO { get; }
    /// <summary>이 FILE만 소유하는 실제 키 물리다.</summary>
    private byte[] _key = [];
    /// <summary>중복 scope 반납만 막는 호스트 표식이다.</summary>
    private bool _disposed;
    /// <summary>키 배열과 CRT 이름의 실제 UTF-8 경계다.</summary>
    private static readonly Encoding Utf8 = new UTF8Encoding(false, true);
    /// <summary>실제 wide CRT 열기다. 반환0만 실패다.</summary>
    [DllImport("ucrtbase.dll", CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    private static extern nint _wfopen(string name, string mode);
    /// <summary>실제 size_t 부분 읽기다.</summary>
    [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    private static extern nuint fread(nint bytes, nuint size, nuint count, nint file);
    /// <summary>실제 size_t 부분 쓰기다.</summary>
    [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    private static extern nuint fwrite(nint bytes, nuint size, nuint count, nint file);
    /// <summary>Windows long32 위치와 원문 origin이다.</summary>
    [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    private static extern int fseek(nint file, int offset, int origin);
    /// <summary>Windows long32 결과이며 실패-1도 그대로 반환한다.</summary>
    [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    private static extern int ftell(nint file);
    /// <summary>실제 fclose 반환은 원문처럼 무시한다.</summary>
    [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    private static extern int fclose(nint file);
    /// <summary>실제 CRT EOF 상태다.</summary>
    [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    private static extern int feof(nint file);
    /// <summary>현재 CLR 실제 배열을 pin하고 같은 BYTE/offset 주소만 CRT에 빌린다.</summary>
    private static void WithBytes(ReadOnlyMemory<byte> bytes, Action<nint> invoke)
    {
        if (!MemoryMarshal.TryGetArray(bytes, out ArraySegment<byte> array) || array.Array is null) throw new ArgumentException("CRT 실제 배열 물리 미연결", nameof(bytes));
        var pin = GCHandle.Alloc(array.Array, GCHandleType.Pinned); try { invoke(nint.Add(pin.AddrOfPinnedObject(), array.Offset)); } finally { pin.Free(); }
    }
    /// <summary>완료된 key 할당의 같은 실제 범위를 빌린다.</summary>
    private Memory<byte> Key(nuint token, uint size) => token == 1 && size <= _key.Length ? _key.AsMemory(0, checked((int)size)) : throw new ArgumentOutOfRangeException(nameof(token), "CRT 키 토큰/물리 범위");
    /// <summary>원문 ColorMapFile처럼 상태를 구성하고 실제 CRT/키 장치를 준비한다.</summary>
    public WindowsBaseFileDevice()
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("실제 Windows CRT 파일 장치가 필요합니다");
        RawBaseFileIO.ConstructBaseFile(State); IO = new RawBaseFileIOHooks
        {
            Open = (name, mode) => unchecked((nuint)_wfopen(Utf8.GetString(name.Span), mode)), Seek = (file, offset, origin) => { _ = fseek(unchecked((nint)file), offset, origin); }, Tell = file => ftell(unchecked((nint)file)), EofFlags = file => feof(unchecked((nint)file)) != 0 ? 0x10u : 0u,
            Close = file => { _ = fclose(unchecked((nint)file)); },
            Read = (file, bytes, size, count) => { if (size != 0 && count != 0) { if ((ulong)size * count > (ulong)bytes.Length) throw new ArgumentOutOfRangeException(nameof(bytes)); WithBytes(bytes, pointer => { _ = fread(pointer, size, count, unchecked((nint)file)); }); } },
            Write = (file, bytes, size, count) => { if (size != 0 && count != 0) { if ((ulong)size * count > (ulong)bytes.Length) throw new ArgumentOutOfRangeException(nameof(bytes)); WithBytes(bytes, pointer => { _ = fwrite(pointer, size, count, unchecked((nint)file)); }); } },
            AllocateKey = size => { _key = new byte[checked((int)size)]; return 1; }, MutableKey = Key, Key = (token, size) => Key(token, size), FreeKey = token => { if (token != 1) throw new ArgumentOutOfRangeException(nameof(token)); _key = []; },
            Assertion = line => throw new InvalidOperationException("색표 BaseFile 원문 진단 " + line)
        };
    }
    /// <summary>부분 열기/키 할당 실패에서도 현재 실제 FILE과 키를 원문 순서로 정리한다.</summary>
    public void Dispose() { if (_disposed) return; RawBaseFileIO.DestructBaseFile(State, IO); _disposed = true; }
}
