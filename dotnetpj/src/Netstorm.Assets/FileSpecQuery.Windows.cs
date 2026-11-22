using System.Runtime.InteropServices;
using System.Text;

namespace Netstorm.Assets; // Windows10/11의 Unicode 검색만 실제 OS 장치에 연결한다.

/// <summary>동일 FindFirst/Next/Close와 가상 게임 CWD 장치다.</summary>
public sealed partial class FileSpecQuery
{
    /// <summary>원시 경로를 OS에 넘길 때만 엄격히 UTF-8로 해석한다.</summary>
    private static readonly Encoding PathEncoding = new UTF8Encoding(false, true);
    /// <summary>WIN32_FIND_DATAW의 네이티브 필드 순서다.</summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct FindData
    {
        /// <summary>파일 속성 DWORD다.</summary>
        public uint Attributes;
        /// <summary>생성/접근/수정 시각의 실제 FILETIME이다.</summary>
        public System.Runtime.InteropServices.ComTypes.FILETIME Creation, Access, Write;
        /// <summary>파일 크기와 예약 DWORD다.</summary>
        public uint SizeHigh, SizeLow, Reserved0, Reserved1;
        /// <summary>첫 긴 Unicode 파일 이름이다.</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)] public string Name;
        /// <summary>대체 짧은 Unicode 파일 이름이다.</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 14)] public string Alternate;
    }
    /// <summary>원문의 Unicode FindFirst 핸들/출력이다.</summary>
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    private static extern nint FindFirstFileW(string name, out FindData data);
    /// <summary>원문의 Unicode 다음 검색이다.</summary>
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool FindNextFileW(nint handle, out FindData data);
    /// <summary>실패 핸들의 반환도 무시하는 실제 반납이다.</summary>
    [DllImport("kernel32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool FindClose(nint handle);
    /// <summary>실제 기본 장치는 Windows 환경에서만 선택한다.</summary>
    private static void RequireWindows() { if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("FileSpec 실제 Windows 검색 장치가 필요합니다"); }
    /// <summary>이름 변환 실패에서도 방금 얻은 핸들을 반납한다.</summary>
    private static FileSpecFindResult WindowsFind(ReadOnlyMemory<byte> name)
    {
        RequireWindows(); nint handle = FindFirstFileW(PathEncoding.GetString(CString(name).Span), out FindData data);
        try { return new(unchecked((nuint)handle), handle == -1 ? ReadOnlyMemory<byte>.Empty : PathEncoding.GetBytes(data.Name)); }
        catch { _ = FindClose(handle); throw; }
    }
    /// <summary>원문의 반환값 무시/실패 핸들 반납을 유지한다.</summary>
    private static void WindowsClose(nuint handle) { RequireWindows(); _ = FindClose(unchecked((nint)handle)); }
    /// <summary>검색 종료만 nullable 없음이며 정상 이름은 원문 UTF-8 BYTE로 반환한다.</summary>
    private static ReadOnlyMemory<byte>? WindowsNext(nuint handle)
    {
        RequireWindows(); if (!FindNextFileW(unchecked((nint)handle), out FindData data)) return null; return new ReadOnlyMemory<byte>(PathEncoding.GetBytes(data.Name));
    }
    /// <summary>검색 시점에만 가상 게임 폴더를 확정하며 호스트 CWD는 바꾸지 않는다.</summary>
    public static FileSpecQueryHooks DirectoryHooks(string directory) => new()
    {
        Find = name =>
        {
            RequireWindows(); string root = Path.GetFullPath(directory), path = PathEncoding.GetString(CString(name).Span);
            bool drive = path.Length >= 2 && path[1] == ':', unc = path.StartsWith("\\\\", StringComparison.Ordinal) || path.StartsWith("//", StringComparison.Ordinal);
            if (!Path.IsPathFullyQualified(path))
            {
                if (!drive && !unc) path = Path.Combine(root, path.TrimStart('\\', '/'));
                else if (drive && (path.Length == 2 || path[2] is not ('\\' or '/')) && string.Equals(path[..2], Path.GetPathRoot(root)?.TrimEnd('\\', '/'), StringComparison.OrdinalIgnoreCase)) path = Path.Combine(root, path[2..]);
            }
            return WindowsFind(PathEncoding.GetBytes(path));
        },
        Close = WindowsClose, Next = WindowsNext
    };
}
