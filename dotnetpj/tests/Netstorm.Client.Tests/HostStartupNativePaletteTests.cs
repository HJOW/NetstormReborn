using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;

namespace Netstorm.Client.Tests; // 최초 화면과 fort 전환이 선택하는 실제 파일 경계를 검사한다.

/// <summary>현재 설정 경로·clear·준비 순서와 Native 길이 진단의 복귀/실패를 검증한다.</summary>
public sealed class HostStartupNativePaletteTests
{
    /// <summary>설치 자산은 읽기만 하며 사용자 저장을 호출하지 않는다.</summary>
    private static string Data() => GameDataLocator.FindDataDirectory() ?? throw new InvalidOperationException("실제 시작 자산 부재");

    /// <summary>존재하지 않는 현재 경로도 명시 로더에 그대로 전달한다. 값 decoder의 선행 읽기는 허용하지 않는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void CurrentInitialAndFortPathsUseOnlySelectedLoaderInOrder(bool modeSet)
    {
        var startup = new HostStartupScene(Data(), "GamePalSpec=missing-native-1217.col");
        startup.Configuration.Set("GamePalSpec", "current-native-1217.col");
        string initial = startup.Configuration.PathSpec("GamePalSpec", startup.Configuration.PathSpec("fortPal"));
        string fort = "";
        var screen = new Screen(1024, 768);
        if (modeSet) Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
        var events = new List<string>();
        startup.Draw(screen, current =>
        {
            Assert.Same(screen, current); Assert.Equal(["initial"], events); events.Add("prepare");
            startup.Configuration.Set("GamePalSpec", "fort-native-1217.col");
            fort = startup.Configuration.PathSpec("GamePalSpec", startup.Configuration.PathSpec("fortPal"));
        }, current =>
        {
            Assert.Same(screen, current); Assert.Equal(["initial", "prepare", "fort"], events); events.Add("rebuild");
        }, (current, path, clear) =>
        {
            Assert.Same(screen, current); Assert.Equal(modeSet ? 1u : 0u, clear);
            if (events.Count == 0) { Assert.Equal(initial, path); events.Add("initial"); }
            else { Assert.Equal(fort, path); events.Add("fort"); }
        });
        Assert.Equal(["initial", "prepare", "fort", "rebuild"], events);
        startup.Draw(screen, _ => throw new InvalidOperationException("중복 준비"),
            _ => throw new InvalidOperationException("중복 색표"), (_, _, _) => throw new InvalidOperationException("중복 파일"));
        // 독립 자산 요청만 명시했을 때에야 없는 최초 파일의 읽기 오류가 발생한다.
        Assert.ThrowsAny<IOException>(() => _ = startup.Palette);
    }

    /// <summary>현재 길이 진단의 정상 복귀는 같은 saved로 적용을 계속하고 예외는 loaded 쓰기 전 파일을 닫는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void NativeLengthAssertionRespectsReturningOrThrowingPolicy(bool fail)
    {
        if (!OperatingSystem.IsWindows()) return;
        string folder = Directory.CreateTempSubdirectory("Netstorm-NativeLength-").FullName;
        try
        {
            string path = Path.Combine(folder, "invalid.col"); File.WriteAllBytes(path, new byte[775]);
            var files = new GameFileSystem(folder, []); var screen = new Screen(4, 2);
            screen.PaletteLoadedOwner.Value = 42; int assertions = 0;
            // 길이 오류 위치만 대여하며 reader 자체의 제어 흐름은 제품 본체가 실행한다.
            void Report(uint line)
            {
                Assert.Equal(0x940u, line); Assert.Equal(42u, screen.PaletteLoaded); assertions++;
                if (fail) throw new InvalidOperationException("중앙 진단 실패");
            }
            if (fail)
                Assert.Equal("중앙 진단 실패", Assert.Throws<InvalidOperationException>(() =>
                    screen.LoadNativePaletteFile(files, path, 0, () => new WindowsBaseFileDevice(), Report)).Message);
            else screen.LoadNativePaletteFile(files, path, 0, () => new WindowsBaseFileDevice(), Report);
            Assert.Equal(1, assertions); Assert.Equal(fail ? 42u : 1u, screen.PaletteLoaded);
            using var exclusive = File.Open(path, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
            Assert.Equal(775, exclusive.Length);
        }
        finally
        {
            // 검사 자신이 만든 절대 임시 폴더만 반납한다.
            Directory.Delete(folder, true);
        }
    }
}
