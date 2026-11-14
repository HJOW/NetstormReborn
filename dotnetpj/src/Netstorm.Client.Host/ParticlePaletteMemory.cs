using System.Buffers.Binary;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;

// 원본 mode1 색 배열의 요청 크기와 현재 인접DWORD를 실제 Win32 경계로 보존한다.
namespace Netstorm.Client.Host;

/// <summary>색수*4바이트만 확보한다. 추가 snapshot DWORD는 생성색이 아닌 실제 인접 메모리다.</summary>
[SupportedOSPlatform("windows")]
public sealed class ParticlePaletteMemory : IDisposable
{
    /// <summary>현재 객체가 소유한 전용 Win32 heap이다.</summary>
    private nint _heap;
    /// <summary>정확히 요청 색수만큼 할당한 실제 주소다.</summary>
    private nint _allocation;
    /// <summary>snapshot 추가 읽기 칸을 포함하지 않는 실제 색 수다.</summary>
    public int Count { get; private set; }
    /// <summary>읽기 전용 경계 검증에 빌려주는 현재 실제 주소다.</summary>
    public nint Address => _allocation;
    /// <summary>OS에서 읽은 색수+1 DWORD를 같은 배열에 보관한다.</summary>
    private uint[] _readable = [];
    /// <summary>명시 반납 뒤 자원 재생성을 막는 관리 수명 표식이다.</summary>
    private bool _disposed;

    /// <summary>첫 Assign까지 heap을 만들지 않는다.</summary>
    public ParticlePaletteMemory() { }
    /// <summary>명시 반납을 놓친 경우에도 실제 unmanaged heap 소유권을 반납한다.</summary>
    ~ParticlePaletteMemory() => Release();

    /// <summary>HeapAlloc(heap,0,count*4) 뒤 실제 색만 쓴다. 다음DWORD를 초기화하지 않는다.</summary>
    public void Assign(ReadOnlySpan<uint> colors)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (colors.Length == 0 || colors.Length > 256) throw new ArgumentException("입자 색 배열 요청 수 오류", nameof(colors));
        if (_heap == 0)
        {
            _heap = HeapCreate(0, 0x1000, 0);
            if (_heap == 0) throw new OutOfMemoryException("입자 실제 heap 생성 실패");
        }
        if (_allocation != 0) { _ = HeapFree(_heap, 0, _allocation); _allocation = 0; }
        Count = 0; _allocation = HeapAlloc(_heap, 0, (nuint)(colors.Length * 4));
        if (_allocation == 0) throw new OutOfMemoryException("입자 실제 색 배열 확보 실패");
        // 요청한 실제 색 DWORD만 쓰며 인접 읽기 칸에 접근하지 않는다.
        for (int index = 0; index < colors.Length; index++) Marshal.WriteInt32(_allocation, index * 4, unchecked((int)colors[index]));
        Count = colors.Length; _readable = new uint[Count + 1];
    }
    /// <summary>현재 배열과 다음DWORD를 OS에서 읽는다. 실패를 마지막색이나0으로 대신하지 않는다.</summary>
    public ReadOnlyMemory<uint> Snapshot()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_allocation == 0 || Count == 0) throw new InvalidOperationException("입자 색 배열 생성 전 조회");
        byte[] bytes = new byte[_readable.Length * 4];
        if (!ReadProcessMemory(GetCurrentProcess(), _allocation, bytes, (nuint)bytes.Length, out nuint received) || received != (nuint)bytes.Length)
            throw new Win32Exception(Marshal.GetLastWin32Error(), "입자 인접 메모리 읽기 실패");
        // 실제 little endian DWORD를 보관하며 추가 칸도 OS 관찰 그대로 유지한다.
        for (int index = 0; index < _readable.Length; index++) _readable[index] = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(index * 4));
        return _readable;
    }
    /// <summary>배열 다음 전용heap 순서로 한 번만 반납한다. 최종화 경로에서도 예외를 만들지 않는다.</summary>
    private void Release()
    {
        if (_disposed) return;
        if (_allocation != 0) _ = HeapFree(_heap, 0, _allocation);
        if (_heap != 0) _ = HeapDestroy(_heap);
        _allocation = _heap = 0; Count = 0; _readable = []; _disposed = true;
    }
    /// <summary>실제 프로세스들이 색을 빌린 수명이 끝난 뒤 소유 배열과heap을 반납한다.</summary>
    public void Dispose() { Release(); GC.SuppressFinalize(this); }

    /// <summary>원문0플래그 전용heap을 만든다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern nint HeapCreate(uint options, nuint initial, nuint maximum);
    /// <summary>원문 요청 바이트 수 그대로 실제 배열을 확보한다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern nint HeapAlloc(nint heap, uint flags, nuint bytes);
    /// <summary>소유heap에 같은 실제 배열을 반납한다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool HeapFree(nint heap, uint flags, nint allocation);
    /// <summary>소유 배열을 비운 전용heap을 반납한다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool HeapDestroy(nint heap);
    /// <summary>현재 호스트의 메모리 관찰용 의사 핸들이다.</summary>
    [DllImport("kernel32.dll")]
    private static extern nint GetCurrentProcess();
    /// <summary>C# 배열 범위 밖 접근 없이 실제 인접DWORD까지 OS가 읽게 한다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool ReadProcessMemory(nint process, nint address, [Out] byte[] bytes, nuint length, out nuint received);
}
