using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 실제 화면·Renderer·클라우드로 미션 월드의 앞단·수집·제출을 매 프레임 구동한다.
namespace Netstorm.Client.Host;

/// <summary>같은 실제 화면·Renderer·카메라 장치로 미션 월드를 매 프레임 합성한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>앞단 행 표·카운터의 현재 저장소다. 미션마다 새로 만든다.</summary>
    private RendererPreludeState? _worldPrelude;
    /// <summary>구름 dirty 4칸의 현재 저장소다. 미션마다 새로 만든다.</summary>
    private RendererCloudQueue? _worldCloudQueue;
    /// <summary>타일 색표·특수 모드의 현재 묶음이다. 팔레트가 바뀌면 다시 만든다.</summary>
    private RawTileDrawModes? _worldTileModes;
    /// <summary>타입 로딩 순서의 실제 SHP 저장소다. 미션마다 새로 만든다.</summary>
    private ShapeDatabase? _worldShapesDb;
    /// <summary>색표를 다시 만들지 않아도 되는 마지막 논리 팔레트다.</summary>
    private uint[]? _worldLogicalPalette;
    /// <summary>앞단이 읽는 화면·카메라·지도 값 상자다. 순서대로 가로·세로·초기높이·카메라Y·지도LTRB다.</summary>
    private readonly int[] _worldScreenBox = new int[8];
    /// <summary>행 표 최초 구성에만 쓰는 별도 초기 화면 높이다. 미션 시작 때 잡는다.</summary>
    private int _worldInitialHeight;

    /// <summary>미션 시작에 앞단·구름·SHP·타일 모드 장치를 묶는다. 수집기는 세션 조립에서 잇는다.</summary>
    /// <param name="shpRaw">타입 로딩 순서의 SHP 원본 바이트다.</param>
    /// <remarks>2026-11-26 1.073에서 R5-3c3 프레임 구동용으로 추가했다. 근거: cpppj ClientRendererFrame의 수집기 재생성과 DrawEntry 표시기 소유다. 배경·제출의 실제 장치는 매 프레임 조회한다.</remarks>
    private void BindMissionWorldDrawing(byte[] shpRaw)
    {
        ArgumentNullException.ThrowIfNull(shpRaw);
        _worldPrelude = new RendererPreludeState(new uint[19]);
        _worldCloudQueue = new RendererCloudQueue();
        _worldShapesDb = new ShapeDatabase(shpRaw);
        _worldInitialHeight = _driver.Screen.Height;
        _worldLogicalPalette = null;
        RefreshWorldTileModes();
    }

    /// <summary>팔레트가 바뀌면 타일 색표를 다시 만들고 매 프레임 특수·로컬 값을 갱신한다.</summary>
    private void RefreshWorldTileModes()
    {
        if (_missionSession is not { } mission || _worldShapesDb is null)
        {
            return;
        }
        uint[] logical = _driver.Screen.LogicalPalette.ToArray();
        if (_worldLogicalPalette is null || !logical.AsSpan().SequenceEqual(_worldLogicalPalette))
        {
            SpriteColorMaps maps = SpriteColorMapBuilder.BuildSpriteColorMaps(logical);
            _worldLogicalPalette = logical;
            _worldTileModes = new RawTileDrawModes
            {
                OwnerMaps = new byte[_surfaceGlobals.Storage.Types.Count],
                OwnerColorsByType = (type, owner) =>
                {
                    if (owner >= mission.BridgeState.OwnerColors.Length) throw new ArgumentOutOfRangeException(nameof(owner), "월드 소유자 범위 오류");
                    int color = mission.BridgeState.OwnerColors[(int)owner];
                    if (color < 0 || color > 8) throw new ArgumentOutOfRangeException(nameof(owner), "월드 소유자 색 번호 오류");
                    return ColorSortInput.OwnerTables.Color(type, (uint)color);
                },
                BridgeLifeNumbers = () => mission.Gameplay.BridgeLifeNumbers.Value,
                FilteredFlags = 2421574144u,
                WideOwnerType = 164,
                UnfilteredType = 84,
                ShadowTable = maps.Shadow,
                DeadTable = maps.Dead,
                AbstractTable = maps.Bright,
            };
        }
        if (_worldTileModes is not null)
        {
            ReadOnlySpan<uint> tokens = ColorSortInput.OwnerTables.Tokens.Span;
            if (tokens.Length != _worldTileModes.OwnerMaps.Length) throw new InvalidOperationException("월드 소유자 색표 TYPE 수 불일치");
            // 옵션에 따라 바뀌는 현재 TYPE별 색표 존재 여부를 매 프레임 읽는다.
            for (int index = 0; index < tokens.Length; index++) _worldTileModes.OwnerMaps[index] = tokens[index] == 0 ? (byte)0 : (byte)1;
            _worldTileModes.SpecialMode = _missionSession.Gameplay.HideBuildings.Value;
            _worldTileModes.LocalPlayer = _missionSession.MissionPlayer.BattleOwner;
        }
    }

    /// <summary>미션 월드의 앞단·수집·제출을 실제 화면 버퍼에 합성한다. 메뉴 그리기는 뒤에 덮는다.</summary>
    /// <remarks>2026-11-26 1.073에서 R5-3c3 프레임 구동용으로 추가했다. 근거: cpppj ClientRendererFrame의 Prepare→Drawing→Finish 순서다. 출력은 기존 전체 텍스처 경로에 맡기고 dirty만 여기서 비운다. override 게이트·묶음 숨김·소유자 색표는 R5-4·R6에서 잇는다.</remarks>
    private void RenderMissionWorldFrame()
    {
        if (_missionSession is not { } mission || _startup is null)
        {
            return;
        }
        if (_worldPrelude is null || _worldCloudQueue is null || _worldShapesDb is null)
        {
            return;
        }
        RefreshWorldTileModes();
        if (_worldTileModes is null)
        {
            return;
        }
        _worldScreenBox[0] = _driver.Screen.Width;
        _worldScreenBox[1] = _driver.Screen.Height;
        _worldScreenBox[2] = _worldInitialHeight;
        _worldScreenBox[3] = mission.Gameplay.Camera.Snapshot.Pixels[1];
        ScreenRect map = NativeMapView();
        _worldScreenBox[4] = map.Left; _worldScreenBox[5] = map.Top; _worldScreenBox[6] = map.Right; _worldScreenBox[7] = map.Bottom;
        ClientKencloudGlobals clouds = _startup.Cloud;
        int cameraX = mission.Gameplay.Camera.Snapshot.Pixels[0];
        int cameraY = _worldScreenBox[3];
        int denominator = mission.Gameplay.Camera.Denominator.Current;
        var cloudHooks = new RendererCloudHooks
        {
            Assertion = line => ErrorLog.Note($"구름 dirty 진단 {line}"),
            Clear = (rect, color) => _driver.Screen.Clear(rect, unchecked((byte)color)),
            Draw = (rect, x, y, alternate) => clouds.Draw(_driver.Screen,
                [unchecked((uint)rect.Left), unchecked((uint)rect.Top), unchecked((uint)rect.Right), unchecked((uint)rect.Bottom), unchecked((uint)x), unchecked((uint)y)],
                _startup.Files, _startup.Configuration, _gameRandom, alternate),
        };
        var prelude = new RendererPreludeState(_worldPrelude.Scan);
        var globals = new RendererPreludeGlobals
        {
            Mode = _driver.Screen.Flags,
            Force = clouds.WorldRedraw,
            Width = new(_worldScreenBox, 0),
            Height = new(_worldScreenBox, 1),
            InitialHeight = new(_worldScreenBox, 2),
            CameraY = new(_worldScreenBox, 3),
            Left = new(_worldScreenBox, 4),
            Top = new(_worldScreenBox, 5),
            Right = new(_worldScreenBox, 6),
            Bottom = new(_worldScreenBox, 7),
        };
        var hooks = new RendererPreludeHooks
        {
            MarkCloudDirty = () => clouds.MarkDirty(),
            QueueCloudDirty = rect => RawRendererClouds.AppendRendererCloudDirty(_worldCloudQueue,
                rect, unchecked((uint)_gameplay.AiDebug.Checking.Value), cloudHooks),
            OverrideActive = () => 0,
            UpdateClouds = () => clouds.Update(_startup.Files, _startup.Configuration, _gameRandom),
            RowDirty = (row, y) => clouds.RowDirty(unchecked((uint)row), unchecked((uint)y)),
            Invalidate = (rect, flags) => NativeRenderer.Invalidate(rect, flags),
            InvalidateBackground = () => RawRendererClouds.RefreshRendererCloudBackground(_worldCloudQueue,
                cameraX, cameraY, denominator, cloudHooks),
            ClearDirty = () => NativeRenderer.ClearDirty(),
        };
        RawRendererPrelude.PrepareRendererFrame(prelude, globals, hooks);
        // 같은 Renderer+60·+64를 앞단 뒤 현재 값으로 둔다. 직접 DWORD 표를 쓰는 caller가 아니므로 ClearDirty는 위 갈고리로 끝냈다.
        NativeRenderer.BackgroundRedraw.Value = prelude.BackgroundRedraw;
        _worldPrelude = prelude;
        uint black = _driver.Screen.Colors().Basic[0];
        uint tint = _missionAudio?.Scene.Tint ?? 167u;
        byte[] buffer = _driver.Screen.Lock();
        try
        {
            var drawing = new RendererWorldDrawing
            {
                Collect = (clip, flags) => mission.CollectWorldDrawing(clip).BackgroundCovered ? 1u : 0u,
                Background = (clip, flags, covered) =>
                {
                    ScreenRect view = NativeMapView();
                    ScreenRect cut = ScreenGeometry.ClipScreenRect(clip, view);
                    RawRendererClouds.DrawRendererBackdrop(cut, flags, covered, new RendererBackdropGlobals
                    {
                        Mode = _driver.Screen.Flags,
                        HideGroups = 0,
                        Tint = tint,
                        Black = black,
                        CameraX = cameraX,
                        CameraY = cameraY,
                        Denominator = denominator,
                    }, cloudHooks);
                },
                Submit = (span, pitch, height, clip) => mission.SubmitWorldDrawing(buffer, pitch, height, clip, _worldShapesDb, _worldTileModes),
            };
            NativeRenderer.Draw(buffer, _driver.Screen.Pitch, 0, drawing);
        }
        finally
        {
            _driver.Screen.Unlock();
        }
        NativeRenderer.Present(_ => { });
        clouds.CleanDirty();
    }
}
