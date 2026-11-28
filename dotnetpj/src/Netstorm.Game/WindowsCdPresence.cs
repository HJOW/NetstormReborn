using System.Buffers;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Assets;

// 같은 원문 CD 판단에 실제 Windows 드라이브·파일 속성 장치를 제공한다.
namespace Netstorm.Client.Host;

/// <summary>ASCII 드라이브 열거와 UTF-8 BYTE 경로의 W API 조회를 원문 장치 경계에 연결한다.</summary>
public static class WindowsCdPresence
{
    /// <summary>잘못된 UTF-8 경로를 다른 파일 이름으로 바꾸지 않는 실제 decoder다.</summary>
    private static readonly UTF8Encoding NativeUtf8 = new(false, true);

    /// <summary>현재 파일 이름 고리를 빌리며 생성 자체는 OS·경로·캐시를 조회하지 않는다.</summary>
    public static CdPresenceHooks CreateHooks(RawFileName names)
    {
        ArgumentNullException.ThrowIfNull(names);
        return new()
        {
            Path = (root, marker) => CdPresence.JoinRootPath(names, root, marker), Exists = Exists,
            Roots = output => CdPresence.CollectRoots(CdPresence.BufferSize, output, new byte[CdPresence.BufferSize], new()
            {
                LogicalDrives = LogicalDrives, DriveType = DriveType,
                Assertion = line => throw new InvalidOperationException($"원본 Winutil.cpp CD 진단 {line}"),
            }),
        };
    }
    /// <summary>캐시/루트가 빌린128BYTE에 직접 OS MULTI_SZ를 쓰고 미작성 꼬리는 유지한다.</summary>
    private static unsafe uint LogicalDrives(Memory<byte> output)
    {
        RequireWindows(); using MemoryHandle pin = output.Pin();
        return GetLogicalDriveStrings(CdPresence.BufferSize, (nint)pin.Pointer);
    }
    /// <summary>현재 루트 BYTE의 NUL 종료 사본을 원문 A API에 전달한다.</summary>
    private static uint DriveType(ReadOnlyMemory<byte> root)
    {
        RequireWindows(); byte[] terminated = new byte[root.Length + 1]; root.Span.CopyTo(terminated);
        return GetDriveType(terminated);
    }
    /// <summary>빈 매체 오류 창을 현재 스레드에서만 억제하고 정상·예외 뒤 원래 모드를 복구한다.</summary>
    private static bool Exists(string rawPath)
    {
        RequireWindows(); bool changed = SetThreadErrorMode(GetThreadErrorMode() | 1u, out uint previous);
        try
        {
            string path = NativeUtf8.GetString(Encoding.Latin1.GetBytes(rawPath));
            return GetFileAttributes(path) != uint.MaxValue;
        }
        finally { if (changed) _ = SetThreadErrorMode(previous, out _); }
    }
    /// <summary>현재 Windows 전용 실제 장치 경계를 다른 OS에서 성공으로 대체하지 않는다.</summary>
    private static void RequireWindows() { if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 CD 장치는 Windows 전용입니다"); }
    /// <summary>GetLogicalDriveStringsA의 실제128BYTE·DWORD 반환 경계다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "GetLogicalDriveStringsA", ExactSpelling = true)]
    private static extern uint GetLogicalDriveStrings(uint length, nint output);
    /// <summary>원문 종류5의 판정에 사용하는 실제 ASCII 루트 조회다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "GetDriveTypeA", ExactSpelling = true)]
    private static extern uint GetDriveType(byte[] root);
    /// <summary>디렉터리를 포함한 실제 Windows UTF-16 파일 속성을 읽는다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "GetFileAttributesW", ExactSpelling = true, CharSet = CharSet.Unicode)]
    private static extern uint GetFileAttributes(string path);
    /// <summary>호출 스레드의 현재 오류 모드만 조회한다.</summary>
    [DllImport("kernel32.dll", ExactSpelling = true)]
    private static extern uint GetThreadErrorMode();
    /// <summary>호출 스레드 오류 모드를 교체하고 실제 이전 모드/성공 여부를 반환한다.</summary>
    [DllImport("kernel32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetThreadErrorMode(uint mode, out uint previous);
}
