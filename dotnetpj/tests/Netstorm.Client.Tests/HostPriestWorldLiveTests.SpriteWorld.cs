using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 실제 게임 자산과 현재 미션 세션을 같은 객체 출력기에 연결한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 사제·셰이프·색표·프로세스가 한 화면 출력에 도달하는지 검증한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제 사제의 현재 위치와 소유자 색표로 SHP를 픽셀에 합성한다.</summary>
    [Fact]
    public void SpriteWorldRendererDrawsActualPriestFromSameSession()
    {
        List<SquidDisplayShape>? shapes = null;
        var (session, types) = Scene(inspectShapes: value => shapes = value);
        int priest = PlacePriest(session, types);
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        var database = new Netstorm.Client.Display.ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp"));
        var input = new ClientColorSortInput();
        uint[] logical = Enumerable.Range(0, 256).Select(index => (uint)index * 0x010101).ToArray();
        input.CaptureWithPrimary(logical, new byte[0x3800]);
        input.Rebuild(0);
        input.OwnerTables.Initialize(input.Primary, types.Count, 82, new UIntBox());
        SpriteDrawState state = SpriteSessionState.Create(input.OwnerTables, () => 6);
        SpriteSessionState.Refresh(state, 0, 0, 1, 0, false, session.BridgeState.OwnerColors,
            PaletteColors.BuildPaletteColorTable(logical), 0);
        var binding = new SpriteColorBinding(input, owner => session.BridgeState.OwnerColors[(int)owner]);
        var renderer = new SpriteWorldRenderer(session.Pool, types, shapes!, definitions, session.WorldFrames, database,
            state, binding, session.BuildSpriteWorldRules(session.TerrainPopHash!, types));
        ReadOnlySpan<byte> raw = session.Pool.Slot(priest);
        float x = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]);
        float y = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
        byte[] pixels = new byte[1024 * 768];
        renderer.Draw(new(x, y, (uint)(priest * RawSidPool.Stride), 0, 0, 0), pixels, 1024, 768, new(0, 0, 1024, 768));
        Assert.Contains(pixels, pixel => pixel != 0);
        var modes = new RawTileDrawModes
        {
            // 타일 단독 출력기에 필요한 표를 비워 공통 객체 출력 경로를 검증한다.
            OwnerMaps = [],
            OwnerColorsByType = (type, owner) => binding.Color(SpriteShapeMode.Owner, type, owner),
            BridgeLifeNumbers = () => session.Gameplay.BridgeLifeNumbers.Value,
            ShadowTable = input.Maps[7], DeadTable = input.Maps[6], AbstractTable = input.Maps[4],
        };
        session.BuildWorldDrawingDevices(session.TerrainPopHash!, session.Pool, types, shapes!, () => new(0, 0, 1024, 768), 1024, 768);
        byte[] submitted = new byte[pixels.Length];
        session.SubmitWorldDrawing(submitted, 1024, 768, new(0, 0, 1024, 768), database, modes, renderer);
        Assert.Contains(Enumerable.Range(0, pixels.Length), index => pixels[index] != 0 && submitted[index] == pixels[index]);
        session.ReleasePlayerProcesses();
    }
}
