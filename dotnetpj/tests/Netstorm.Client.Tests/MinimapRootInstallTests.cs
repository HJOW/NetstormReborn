using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// RU-4a 실제 Root 미니맵 설치 계약이다. 0.991에서 추가됐다.
public sealed class MinimapRootInstallTests
{
    /// <summary>창이나 음향을 열지 않고 원본 자산과 같은 장기 저장소·공통 트리를 구성한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>동일 실제 팔레트와 8비트 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>수신 옵션과 HTML 생성기가 공유하는 실제 시작 설정이다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>같은 실제 메뉴와 공통 노드 트리다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>실제 PlayerKnown과 Surface의 권한·보조 owner다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 실제 자산·현재 모드 저장소다.</summary>
        public readonly RawSurfaceStorage Storage;
        /// <summary>같은 공유 bitmap·큐다.</summary>
        public readonly MetadisplayDirtyState State = new();
        /// <summary>같은 전용 팔레트·현재 토큰·serial이다.</summary>
        public readonly RawMinimapGlobals Globals = new();
        /// <summary>선택 이동의 현재 모드다. R5 입력 연결 전까지 거부한다.</summary>
        public bool Gated = true;
        /// <summary>같은 실제 장기 미니맵 처리기다. Open 대신 Construct로 만든다.</summary>
        public readonly RawMinimapControls Mini;

        /// <summary>제품 설치와 같은 주인·저장소·전역·후크 모양으로 조립한다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Storage = new(OriginalEdition.Patch1078, Game.Authority, Game.GameAux); _ = new HostAssetTables(Startup, Storage);
            var baseHandle = Menu.Tree.EventHooks.Handle;
            Mini = new(Menu.Tree, Storage, State, Globals, Game.MapRevision, new()
            {
                View = () => new MinimapGeometryView(
                    BinaryPrimitives.ReadUInt32LittleEndian(BitConverter.GetBytes((float)Game.Camera.Snapshot.Pixels[0])),
                    BinaryPrimitives.ReadUInt32LittleEndian(BitConverter.GetBytes((float)Game.Camera.Snapshot.Pixels[1])),
                    Menu.CurrentMapView),
                Center = (_, _) => throw new InvalidOperationException("미니맵 중앙 카메라는 R5 카메라 연결에서 잇는다"),
                Move = (_, _) => throw new InvalidOperationException("미니맵 선택 이동은 R5 입력 연결에서 잇는다"),
                InputMode = () => Gated ? throw new InvalidOperationException("미니맵 입력 모드는 R5 입력 연결에서 잇는다") : 0u,
                Poll = () => throw new InvalidOperationException("미니맵 입력 폴링은 R5 입력 연결에서 잇는다"),
                BaseHandle = (token, code, first, second) => (baseHandle ?? throw new InvalidOperationException("미니맵 실제 기본 입력 미설치"))(token, code, first, second),
                Invalidate = (_, _) => { },
                Changed = () => { },
            });
            Mini.Construct();
            Mini.SetPalette(Screen.Colors());
        }

        /// <summary>자기 UI만 반납한다. 물리·전역은 따로 소유한다.</summary>
        public void Dispose() => Mini.Dispose();
    }

    /// <summary>설치는 같은 Gameplay.MapRevision을 공유 갱신 DWORD로 둔다.</summary>
    [Fact]
    public void InstallSharesGameplayMapRevision()
    {
        using var scene = new Scene();
        Assert.Same(scene.Game.MapRevision, scene.State.SharedFullRefresh);
        scene.Game.MapRevision.Value = 7;
        Assert.Equal(7u, scene.State.FullRefresh);
    }

    /// <summary>생성은 현재 bitmap 폭을 쓰고 Open 없이 끝난다.</summary>
    [Fact]
    public void ConstructUsesBitmapWidthWithoutOpen()
    {
        using var scene = new Scene();
        Assert.NotEqual(0u, scene.Mini.RootToken);
        Assert.Equal(scene.Mini.RootToken, scene.Globals.Current);
        Assert.Empty(scene.State.Bitmap);
    }

    /// <summary>같은 현재 지도 getter를 미니맵 보기가 다시 읽는다.</summary>
    [Fact]
    public void ViewReusesCurrentMapGetter()
    {
        using var scene = new Scene();
        Assert.Equal(scene.Menu.CurrentMapView, scene.Menu.Tree.TreeState.MapView);
    }

    /// <summary>중복 생성과 미연결 장치는 명시 거부한다.</summary>
    [Fact]
    public void DuplicateConstructAndMissingHooksFailExplicitly()
    {
        using var scene = new Scene();
        Assert.Throws<InvalidOperationException>(() => scene.Mini.Construct());
        Assert.Throws<ArgumentException>(() => new RawMinimapControls(scene.Menu.Tree, scene.Storage, scene.State, scene.Globals, new(), new()
        {
            View = () => default, Center = (_, _) => { }, Move = (_, _) => { }, InputMode = () => 0, Poll = () => default,
            BaseHandle = (_, _, _, _) => 0, Invalidate = (_, _) => { }, Changed = () => { },
        }));
    }

    /// <summary>클릭 입력은 선택 모드 조회까지 도달하고 R5 연결을 명시 요구한다.</summary>
    [Fact]
    public void ClickReachesInputModeGate()
    {
        using var scene = new Scene();
        var error = Assert.Throws<InvalidOperationException>(() => scene.Mini.Handle(scene.Mini.RootToken, 0x10000, 10, 10));
        Assert.Contains("R5", error.Message);
    }

    /// <summary>반납 뒤에는 같은 등록이 남지 않는다.</summary>
    [Fact]
    public void DisposeReleasesRegistration()
    {
        var scene = new Scene();
        int before = scene.Menu.Tree.ExternalDrawing.RegistrationCount;
        scene.Dispose();
        Assert.Equal(before - 1, scene.Menu.Tree.ExternalDrawing.RegistrationCount);
    }
}
