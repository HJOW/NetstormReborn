using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;

// 실제 Windows 속성·드라이브와 현재 스레드 오류 모드 복구를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>원문 판단 본체의 합성 장치 검사와 실제 Windows 장치 검사를 구별한다.</summary>
public sealed class WindowsCdPresenceTests
{
    /// <summary>CPP UTF-8 경로와 같은 원문 BYTE 문자열 뷰를 만든다.</summary>
    private static string Narrow(string value) => Encoding.Latin1.GetString(Encoding.UTF8.GetBytes(value));

    /// <summary>한글/보충문자 경로의 파일·디렉터리 속성과 예외 뒤 스레드 모드를 확인한다.</summary>
    [Fact]
    public void UnicodeMarkerAndFailureRestoreThreadErrorMode()
    {
        if (!OperatingSystem.IsWindows()) return;
        string root = Path.Combine(Path.GetTempPath(), "NetstormCD1226-" + Guid.NewGuid().ToString("N") + "한글😀");
        string marker = Path.Combine(root, CdPresence.Marker); Directory.CreateDirectory(root);
        uint previous = GetThreadErrorMode(); bool changed = SetThreadErrorMode(previous | 0x8000u, out _);
        try
        {
            uint expected = GetThreadErrorMode(); var names = new RawFileName(); CdPresenceHooks hooks = WindowsCdPresence.CreateHooks(names);
            Assert.Equal(0, names.Index); string path = hooks.Path!(Encoding.UTF8.GetBytes(root), CdPresence.Marker);
            File.WriteAllText(marker, "", Encoding.UTF8); Assert.True(hooks.Exists!(path)); Assert.Equal(expected, GetThreadErrorMode());
            File.Delete(marker); Directory.CreateDirectory(marker); Assert.True(hooks.Exists(path)); Assert.Equal(expected, GetThreadErrorMode());
            Assert.False(hooks.Exists(Narrow(Path.Combine(root, "missing")))); Assert.Equal(expected, GetThreadErrorMode());
            Assert.Throws<DecoderFallbackException>(() => hooks.Exists("\u00ff")); Assert.Equal(expected, GetThreadErrorMode());
            var owner = new ClientCdGlobals(); Encoding.UTF8.GetBytes(root).CopyTo(owner.CachedRoot.Span);
            Assert.Equal(1u, owner.Check(new() { Path = hooks.Path, Exists = hooks.Exists, Roots = _ => throw new InvalidOperationException("실제 속성 캐시 성공은 드라이브를 열거하지 않는다") }));
        }
        finally
        {
            if (changed) _ = SetThreadErrorMode(previous, out _);
            if (File.Exists(marker)) File.Delete(marker);
            if (Directory.Exists(marker)) Directory.Delete(marker);
            Directory.Delete(root);
        }
    }

    /// <summary>실제 논리 드라이브의 CD-ROM 집합을 독립 DriveInfo 조회와 비교한다.</summary>
    [Fact]
    public void ActualRootsMatchWindowsCdRomDrives()
    {
        if (!OperatingSystem.IsWindows()) return;
        byte[] output = Enumerable.Repeat((byte)205, 128).ToArray();
        uint result = WindowsCdPresence.CreateHooks(new RawFileName()).Roots!(output);
        string[] expected = DriveInfo.GetDrives().Where(drive => drive.DriveType == DriveType.CDRom).Select(drive => drive.Name).Order(StringComparer.Ordinal).ToArray();
        if (expected.Length == 0) { Assert.Equal(0u, result); Assert.All(output, value => Assert.Equal(205, value)); return; }
        Assert.InRange(result, 1u, 128u); var actual = new List<string>(); int cursor = 0;
        // 실제 MULTI_SZ의 각 NUL 종료 루트를 읽으며 최종 이중NUL에서 멈춘다.
        while (output[cursor] != 0)
        {
            int length = output.AsSpan(cursor).IndexOf((byte)0); Assert.True(length >= 0);
            actual.Add(Encoding.ASCII.GetString(output, cursor, length)); cursor += length + 1;
        }
        Assert.Equal(expected, actual.Order(StringComparer.Ordinal)); Assert.Equal((uint)cursor + 1, result);
    }

    /// <summary>현재 테스트 스레드의 실제 오류 모드를 조회한다.</summary>
    [DllImport("kernel32.dll", ExactSpelling = true)]
    private static extern uint GetThreadErrorMode();
    /// <summary>현재 테스트 스레드만 잠시 변경하고 이전 모드로 복구한다.</summary>
    [DllImport("kernel32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetThreadErrorMode(uint mode, out uint previous);
}
