using Netstorm.Assets;
using Netstorm.Core.Original;

// 첫메뉴앞에서실제자산을한번읽고모든Session이같은현재저장소를빌리게한다.
namespace Netstorm.Client.Host;

/// <summary>GameAssets/MakeRawSurfaceStorage의TYPE·Frame·SHP소유자연결이다. 화면/Runtime/Player를생성하지않는다.</summary>
public sealed class HostAssetTables : IRawSurfaceAssetSource
{
    /// <summary>같은Client의실제장기저장소다.</summary>
    private readonly RawSurfaceStorage _storage;
    /// <summary>원본의최초TYPE이름/번호/적재값이다. 현재Storage.Types사본과구분한다.</summary>
    public RiftTypeTable TypeTable { get; }
    /// <summary>같은최초적재정의다. 미션마다다시읽지않는다.</summary>
    public IReadOnlyList<RiftTypeDefinition> Definitions { get; }
    /// <summary>같은최초SHP파일바이트다. 소비자는이동일소유자를읽기전용으로빌린다.</summary>
    public byte[] ShapeImage { get; }
    /// <summary>현재저장소의동일프레임물리배열이다.</summary>
    public RiftTypeFrames[] Frames => _storage.Frames.PhysicalArray;
    /// <summary>현재저장소의동일셰이프객체를빌린다.</summary>
    public List<SquidDisplayShape> Shapes => _storage.Shapes;
    /// <summary>비용등은수정가능한현재사본대신실제최초자산의TYPE를조회한다.</summary>
    public IReadOnlyList<OriginalTypeRecord> LoadedTypes => TypeTable.Types;

    /// <summary>실제 자산 번호의 icon TYPE를 찾아 현재 공유 프레임 표의 B/C 구간 수를 읽는다.</summary>
    public int IconFrameCount(byte kind)
    {
        int type = TypeTable.Find("icon");
        if (type < 0 || type >= Frames.Length) throw new InvalidDataException("icon 실제 TYPE 프레임 표가 없다");
        return Frames[type].Run(kind).Count;
    }

    /// <summary>실제TYPE/정의/SHP를준비한뒤빈장기물리에한번설치한다. 실패한입력을임의빈표로대체하지않는다.</summary>
    public HostAssetTables(HostStartupScene startup, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(storage);
        if (storage.Pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("실제10.78자산저장소필요", nameof(storage));
        if (storage.Types.Count != 0 || storage.Frames.Count != 0 || storage.Shapes.Count != 0) throw new InvalidOperationException("이미준비된장기자산표재설치");
        _storage = storage; TypeTable = MissionFortLoad.LoadTypeTable(startup.Files); Definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        ShapeImage = startup.Files.ReadAllBytes("d/_shapes.shp"); var shapes = TerrainDisplayShapes.Build(Definitions, new ShapeDatabase(ShapeImage));
        _storage.InitializeAssetTables(TypeTable.Types, WorldFrameTables.Create(Definitions, TypeTable.Types.Count), shapes, this);
    }
}
