using System.Text;
using Netstorm.Assets;

namespace Netstorm.Core.Original; // Source 열기 정책에 현재 등록/가상 CWD/OS 경계를 공급한다.

/// <summary>실제 FILE과 키를 한 scope에서 소유하는 플랫폼 장치다.</summary>
public interface IRawBaseFileDevice : IDisposable
{
    /// <summary>같은 실제 여섯 필드다.</summary>
    RawBaseFileState State { get; }
    /// <summary>현재 FILE/키를 사용하는 같은 실제 장치다.</summary>
    RawBaseFileIOHooks IO { get; }
}

/// <summary>BaseFile.cpp의 현재 root/등록 TAFF/가상 게임 CWD를 RawOpenPolicy에 연결한다. 언어/Probe 상위는 별도다.</summary>
public static class RawBaseFileEnvironment
{
    /// <summary>잘린 UTF-8 BYTE는 디스크 열기 실패이며 TAFF 조회는 유지한다.</summary>
    private static readonly Encoding StrictUtf8 = new UTF8Encoding(false, true);
    /// <summary>현재 root의 실제 절대 경로를 원문 Windows BYTE로 제공한다.</summary>
    private static byte[] Root(string path) => StrictUtf8.GetBytes(Path.GetFullPath(path).Replace('/', '\\'));
    /// <summary>Source OpenPolicy의 현재 환경이다. .korean 대체/Probe/ACP 상위 API를 가장하지 않는다.</summary>
    public static void OpenByPolicy(GameFileSystem files, RawBaseFileState state, ReadOnlyMemory<byte> name, uint mode, uint create, RawBaseFileIOHooks io)
    {
        ArgumentNullException.ThrowIfNull(files); var hooks = new BaseFileOpenPolicyHooks
        {
            PrimaryRoot = () => Root(files.BaseDirectory), SecondaryRoot = () => Root(files.SecondaryDirectory ?? files.BaseDirectory),
            Archive = (query, secondary) => { var item = files.FindArchiveOpen(query, secondary); return item is null ? null : new BaseFileArchiveOpen(StrictUtf8.GetBytes(item.FileName), item.Offset, item.Length, item.Encrypted); },
            Search = FileSpecQuery.DirectoryHooks(files.BaseDirectory),
            UnableToOpen = failed => { Console.WriteLine("unable to open: " + Encoding.UTF8.GetString(failed.Span)); throw new ColorCacheUnableToOpenException(); }
        };
        var devices = new RawBaseFileIOHooks
        {
            Seek = io.Seek, Tell = io.Tell, EofFlags = io.EofFlags, Close = io.Close, FreeKey = io.FreeKey, Key = io.Key, Write = io.Write, Assertion = io.Assertion, Read = io.Read, AllocateKey = io.AllocateKey, MutableKey = io.MutableKey,
            Open = (raw, selectedMode) =>
            {
                var open = io.Open ?? throw new InvalidOperationException("BaseFile 환경의 실제 FILE 장치 미연결"); string path;
                try { path = StrictUtf8.GetString(raw.Span); } catch (DecoderFallbackException) { return 0; }
                string root = Path.GetFullPath(files.BaseDirectory); bool drive = path.Length >= 2 && path[1] == ':', unc = path.StartsWith("\\\\", StringComparison.Ordinal) || path.StartsWith("//", StringComparison.Ordinal);
                if (!Path.IsPathFullyQualified(path))
                {
                    if (!drive && !unc) path = Path.IsPathRooted(path) ? Path.Combine(Path.GetPathRoot(root)!, path.TrimStart('\\', '/')) : Path.Combine(root, path);
                    else if (drive && (path.Length == 2 || path[2] is not ('\\' or '/')) && string.Equals(path[..2], Path.GetPathRoot(root)?.TrimEnd('\\', '/'), StringComparison.OrdinalIgnoreCase)) path = Path.Combine(root, path[2..]);
                }
                return open(StrictUtf8.GetBytes(path), selectedMode);
            }
        };
        RawBaseFileOpenPolicy.OpenBaseFileByPolicy(state, name, mode, create, files.FileNameOwner, devices, hooks);
    }
}
