using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// 구름(Kencloud) 소유자(<see cref="ClientKencloudGlobals"/>)를 실제 자산·설정·난수·화면에 연결해 검사한다.
/// cpppj <c>ClientKencloudGlobalsTests.cpp</c>의 물리 전역·실제 파일·fallback·옵션·팔레트 전환·갱신 검사와,
/// 첫 L4a 비교(0.380)에서 드러난 메인 메뉴 구름 차이를 닫은 이식(0.381)의 직접 검사를 담았다.
/// </summary>
public sealed class ClientKencloudGlobalsTests
{
    /// <summary>실제 배포 자산으로 시작 화면을 만든다(구름은 아직 생성하지 않는다).</summary>
    private static HostStartupScene Scene()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        return new HostStartupScene(data!);
    }

    /// <summary>PE 최초값·인접 별칭·살아 있는 블록의 내부/one-past 접근·반납 뒤 거부를 확인한다.</summary>
    [Fact]
    public void PhysicalGlobalsAndAddressLifetime()
    {
        var cloud = new ClientKencloudGlobals(bytes => bytes.Fill(0x71));
        KencloudGlobals globals = cloud.Globals();
        Assert.Equal(2054, cloud.State().Length);
        Assert.True(globals.Cache == 0 && globals.Phase == 0 && globals.LowColor == 228 && globals.Speed == 2 && globals.Fury == 10 && globals.SeedCount == 400);
        // 구름 이름 버퍼는 뒤의 flags와 같은 물리다. 복사본이면 별칭이 끊어진다.
        cloud.CloudName().Span[84] = 7;
        Assert.Equal(7u, cloud.State()[2049]);
        globals.DirtyRows[3] = 29;
        Assert.Equal(29u, cloud.State()[519]);
        globals.Seeds[999] = 31;
        Assert.Equal(31u, cloud.State()[2027]);
        uint first = cloud.Allocate(36), second = cloud.Allocate(262144);
        Assert.True(second > first + 36);
        Assert.Equal(36, cloud.Memory(first).Length);
        Assert.Equal(1, cloud.Memory(first + 35).Length);
        Assert.Equal(0, cloud.Memory(first + 36).Length); // one-past는 길이 0 이동을 위해 유효하다.
        Assert.Equal(0x71, cloud.Memory(first)[0]); // 할당 입력 장치의 내용이 그대로 들어간다.
        cloud.Memory(second + 512)[0] = 19;
        Assert.Equal(19, cloud.Memory(second)[512]);
        Assert.Throws<ArgumentOutOfRangeException>(() => { _ = cloud.Memory(first + 37 + 0x2000_0000); }); // 어떤 블록에도 속하지 않는 먼 주소는 범위 밖이다.
        cloud.Release(first);
        Assert.Equal(1, cloud.Allocations());
        Assert.Throws<ArgumentOutOfRangeException>(() => { _ = cloud.Memory(first); }); // 해제된 주소는 거부한다.
        Assert.Throws<ArgumentOutOfRangeException>(() => cloud.Release(first)); // 이중 반납도 거부한다.
        cloud.Release(0); // 0은 아무것도 하지 않는다.
    }

    /// <summary>실제 512 GIF의 픽셀 전체·경계·난수 소비·캐시 재조회 자연 반환을 확인한다.</summary>
    [Fact]
    public void ActualFileAndNative400SeedRandom()
    {
        HostStartupScene scene = Scene();
        var random = new GameRandom(0x89abcdefU);
        var pixels = Netstorm.Client.Display.GifImage.DecodeBackgroundGif(scene.Files.ReadAllBytes(scene.Configuration.PathSpec("CloudFileSpec")));
        Assert.True(pixels.Width == 512 && pixels.Height == 512);
        ClientKencloudGlobals cloud = scene.Cloud;
        cloud.Build(scene.Files, scene.Configuration, random);
        KencloudGlobals globals = cloud.Globals();
        Assert.True(globals.Cache != 0 && globals.End == globals.Cache + 262144 && globals.NextRow == globals.Cache + 512 && globals.LastRow == globals.End - 512 && cloud.Allocations() == 1);
        Assert.True(pixels.Indices.AsSpan().SequenceEqual(cloud.Memory(globals.Cache)[..262144]));
        // 시작 설정 400개 seed는 (510,512) 두 번씩, 마지막에 spacer 256 한 번이다. 같은 시작 상태의 독립 난수가 같은 상태에 도달한다.
        var expected = new GameRandom(0x89abcdefU);
        // 시작 seed 400개마다 두 난수를 뽑는다.
        for (int index = 0; index < 400; index++)
        {
            uint y = expected.Next(510) + 1, x = expected.Next(512);
            Assert.Equal(globals.Cache + y * 512 + x, globals.Seeds[index]);
        }
        Assert.Equal(expected.Next(256) + 5, globals.Spacer);
        Assert.Equal(expected.State(), random.State());
        Assert.True(globals.PartialDirty == 0 && globals.AllDirty == 1);
        // 이미 캐시가 있으면 하위(난수·파일·할당)를 모두 생략하고 dirty만 쓴다.
        uint token = globals.Cache, state = random.State(), byteAt = cloud.Memory(token)[37];
        cloud.Build(scene.Files, scene.Configuration, random);
        Assert.True(globals.Cache == token && random.State() == state && cloud.Allocations() == 1 && cloud.Memory(token)[37] == byteAt);
    }

    /// <summary>없는 파일과 512가 아닌 실제 GIF는 반납 뒤 같은 난수·채움으로 fallback한다(색은 low 색, 위상은 warm-up 횟수만큼 진행).</summary>
    /// <param name="spec">구름 파일 경로 설정이다.</param>
    [Theory]
    [InlineData("d/no-kencloud-424.gif")]
    [InlineData("d/Defeat.gif")]
    public void MissingAndWrongSizeUseActualFallback(string spec)
    {
        HostStartupScene scene = Scene();
        scene.Configuration.Configuration.AppendSection("END");
        scene.Configuration.Set("CloudFileSpec", spec);
        var random = new GameRandom(0x89abcdefU);
        ClientKencloudGlobals cloud = scene.Cloud;
        cloud.Build(scene.Files, scene.Configuration, random);
        KencloudGlobals globals = cloud.Globals();
        Assert.True(globals.Cache != 0 && cloud.Allocations() == 1); // 512가 아닌 GIF는 파일 부모가 반납하고 fallback 할당이 하나 남는다.
        Assert.True(cloud.Memory(globals.Cache)[..262144].ToArray().All(value => value == 228)); // low 색으로 채운 뒤 행 이동해도 같은 값뿐이다.
        // warm-up: fury 10 → 10000/10 = 1000번, 각 번 speed(2)번 이동 → 2000번 이동의 위상 합이다.
        Assert.Equal(2000u & 15u, globals.Phase);
        var expected = new GameRandom(0x89abcdefU);
        for (int index = 0; index < 400; index++) { expected.Next(510); expected.Next(512); }
        expected.Next(256);
        Assert.Equal(expected.State(), random.State()); // warm-up은 난수를 더 소비하지 않는다.
    }

    /// <summary>fury 0은 현재 할당·seed 효과를 남기고 정상 dirty reset까지 도달하지 않는다.</summary>
    [Fact]
    public void FallbackDivideFaultPreservesPrefix()
    {
        HostStartupScene scene = Scene();
        scene.Configuration.Configuration.AppendSection("END");
        scene.Configuration.Set("CloudFileSpec", "d/no-kencloud-424.gif");
        ClientKencloudGlobals cloud = scene.Cloud;
        cloud.Value(ClientKencloudGlobals.Option.Fury) = 0;
        var random = new GameRandom(0x89abcdefU);
        Assert.Throws<DivideByZeroException>(() => cloud.Build(scene.Files, scene.Configuration, random));
        KencloudGlobals globals = cloud.Globals();
        Assert.True(globals.Cache != 0 && cloud.Allocations() == 1 && globals.AllDirty == 0 && globals.PartialDirty == 0); // 정상 reset(all=1) 전에 중단했다.
    }

    /// <summary>시작·설정 변경 때 옵션 읽기가 누락 값을 보존하고 signed DWORD를 그대로 적용한다.</summary>
    [Fact]
    public void ActualConfigOptionsKeepMissingAndSignedValues()
    {
        HostStartupScene scene = Scene();
        ClientKencloudGlobals cloud = scene.Cloud;
        cloud.ReadOptions(scene.Configuration);
        Assert.Equal(2u, cloud.Value(ClientKencloudGlobals.Option.Speed)); // 설정에 없으면 최초값을 보존한다.
        scene.Configuration.Configuration.AppendSection("END");
        scene.Configuration.Set("cloudSpeed", "-1");
        scene.Configuration.Set("cloudFury", "-17");
        scene.Configuration.Set("cloudiness", "13");
        scene.Configuration.Set("cloudAlgo", "3");
        scene.Configuration.Set("kenCloud", "1");
        cloud.ReadOptions(scene.Configuration);
        Assert.Equal(0xffffffffU, cloud.Value(ClientKencloudGlobals.Option.Speed));
        Assert.Equal(0xffffffefU, cloud.Value(ClientKencloudGlobals.Option.Fury));
        Assert.Equal(13u, cloud.Value(ClientKencloudGlobals.Option.Cloudiness));
        Assert.Equal(3u, cloud.State()[2050]);
        Assert.Equal(1u, cloud.State()[2049]);
        Assert.Equal(0, cloud.Allocations()); // 옵션 읽기는 파일·난수·구름 생성을 하지 않는다.
    }

    /// <summary>
    /// 메인 메뉴 진입의 팔레트 전환이 구름을 만들고 cloudSpeed(2)번 행을 이동시키며(위상 2), 같은 이름의 재전환·재그리기는 아무것도 다시 하지 않는다.
    /// 이 한 번의 이동이 첫 L4a 비교(0.380)에서 드러난 3행 띠 24개의 1픽셀 어긋남이다.
    /// </summary>
    [Fact]
    public void MenuTransitionBuildsCloudOnceAndStepsSpeedTimes()
    {
        HostStartupScene scene = Scene();
        var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
        KencloudGlobals globals = scene.Cloud.Globals();
        Assert.Equal(0u, globals.Cache);
        uint startState = scene.Random.State();
        scene.Draw(screen);
        Assert.True(globals.Cache != 0 && globals.Phase == 2 && scene.Cloud.Allocations() == 1); // 갱신 횟수 = cloudSpeed(2)
        Assert.NotEqual(startState, scene.Random.State()); // seed 800개 + spacer 1개를 같은 게임 난수에서 뽑았다.
        uint afterFirst = scene.Random.State(), cache = globals.Cache;
        byte[] first = screen.Pixels.ToArray();
        scene.Draw(screen); // 같은 이름 전환은 조기 반환하고 구름은 다시 만들지 않는다.
        Assert.True(globals.Cache == cache && globals.Phase == 2 && scene.Random.State() == afterFirst && scene.Cloud.Allocations() == 1);
        Assert.True(first.AsSpan().SequenceEqual(screen.Pixels)); // 같은 화면이 다시 그려진다.
        Assert.True(globals.AllDirty == 0 && globals.PartialDirty == 0); // 그린 뒤 정리했다.
        Assert.Equal(0u, scene.Cloud.WorldRedraw); // 렌더러가 없으면 전환 뒤 dirty를 쓰지 않는다.
    }

    /// <summary>구름 표시는 카메라 원점 0에서 512 타일을 반복하며 화면 픽셀이 현재 cache 바이트와 같다.</summary>
    [Fact]
    public void DrawTilesCacheAtCameraOrigin()
    {
        HostStartupScene scene = Scene();
        var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
        var random = new GameRandom(7);
        scene.Cloud.Update(scene.Files, scene.Configuration, random); // 생성 후 cloudSpeed번 이동한다.
        scene.Cloud.MarkDirty();
        scene.Cloud.Draw(screen, [0u, 0u, 1024u, 768u, 0u, 0u], scene.Files, scene.Configuration, random);
        scene.Cloud.CleanDirty();
        Span<byte> cache = scene.Cloud.Memory(scene.Cloud.Globals().Cache);
        // 화면 전체의 각 픽셀이 같은 타일 위치의 cache 바이트와 같다.
        for (int y = 0; y < 768; y++)
        {
            for (int x = 0; x < 1024; x++) Assert.Equal(cache[(y & 511) * 512 + (x & 511)], screen.Pixels[y * screen.Pitch + x]);
        }
        Assert.Equal(2u, scene.Cloud.Globals().Phase);
    }

    /// <summary>
    /// 전체 표시 요청이 없고 일부 행만 dirty이면 그 행만 모든 가로·세로 타일에서 복사하고 나머지 화면은 그대로 둔다.
    /// 복사한 행마다 이전 표시 요청(앞 표)이 남는다. 전체 경로에서는 원본 행 번호가 0이라 가려지는 원본 사각형 계산을 이 경로가 확인한다.
    /// </summary>
    [Fact]
    public void PartialDrawBlitsOnlyDirtyRowsInEveryTile()
    {
        HostStartupScene scene = Scene();
        var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
        var random = new GameRandom(7);
        scene.Cloud.Build(scene.Files, scene.Configuration, random);
        byte[] pixels = screen.Lock();
        Array.Fill(pixels, (byte)0xff); // 복사하지 않은 행을 구별하는 표식이다.
        screen.Unlock();
        KencloudGlobals globals = scene.Cloud.Globals();
        globals.AllDirty = 0; // 생성이 남긴 전체 표시 요청을 끄고 일부 행만 요청한다.
        globals.PartialDirty = 0;
        globals.DirtyRows[5] = 1;
        globals.DirtyRows[300] = 1;
        scene.Cloud.Draw(screen, [0u, 0u, 1024u, 768u, 0u, 0u], scene.Files, scene.Configuration, random);
        Span<byte> cache = scene.Cloud.Memory(globals.Cache);
        // 세로 타일 두 장(0, 512)에서 캐시 행 5와 300이 놓이는 화면 행만 복사됐는지 모든 행·열을 확인한다.
        for (int y = 0; y < 768; y++)
        {
            bool dirty = (y & 511) == 5 || (y & 511) == 300;
            for (int x = 0; x < 1024; x++)
                Assert.Equal(dirty ? cache[(y & 511) * 512 + (x & 511)] : (byte)0xff, screen.Pixels[y * screen.Pitch + x]);
        }
        Assert.Equal(1u, scene.Cloud.State()[5]); // 복사한 행은 다음 표시를 위해 이전 표시 요청을 남긴다.
        Assert.Equal(1u, scene.Cloud.State()[300]);
        Assert.Equal(0u, scene.Cloud.State()[6]);
    }

    /// <summary>두 번째 표시 부모(alternate)는 미복원 장치라 명시 거부하고, 면적 0 표시는 장치를 조회하지 않는다.</summary>
    [Fact]
    public void AlternateDrawIsRejectedAndEmptyAreaSkips()
    {
        HostStartupScene scene = Scene();
        var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
        scene.Cloud.Draw(screen, [0u, 0u, 0u, 768u, 0u, 0u], scene.Files, scene.Configuration, scene.Random); // 폭 0이면 캐시 생성도 하지 않는다.
        Assert.Equal(0u, scene.Cloud.Globals().Cache);
        Assert.Throws<InvalidOperationException>(() => scene.Cloud.Draw(screen, [0u, 0u, 16u, 16u, 0u, 0u], scene.Files, scene.Configuration, scene.Random, alternate: true));
    }
}
