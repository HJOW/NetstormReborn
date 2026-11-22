using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 같은 최초 TYPE/정의/SHP를 스타일 명령과 실제 그림 조각이 공유한다.
namespace Netstorm.Client.Gump;

/// <summary>Markup.cpp/StyleTextMarkup.cpp의 GameAssets 어댑터다.소비자가 이미 적재한 실제 자산만 빌린다.</summary>
public sealed class MarkupGameAssets
{
    /// <summary>내장 무자산 슬롯을 포함한 같은 최초 타입 표다.</summary>
    private readonly RiftTypeTable _types;
    /// <summary>같은 최초116개 자산 정의다.조회에서 프레임 선택 필드를 복사한다.</summary>
    private readonly IReadOnlyList<RiftTypeDefinition> _definitions;
    /// <summary>같은 최초 SHP 바이트의 논리/물리 조회 장치다.</summary>
    private readonly ShapeDatabase _shapes;
    /// <summary>스타일 조각과 픽셀 출력이 공유하는 동일 자산 어댑터다.</summary>
    public MarkupAssets Pictures { get; }
    /// <summary>원문 00541348의 실제 최초 타입 수다.</summary>
    public int TypeCount => _types.Types.Count;
    /// <summary>현재 월드 사본과 구별하는 동일 최초 TYPE 표의 읽기 전용 주소다.</summary>
    public IReadOnlyList<OriginalTypeRecord> LoadedTypes => _types.Types;
    /// <summary>같은 공통 UI의 생산/측정도 최초 정의를 복사 없이 빌린다.</summary>
    internal IReadOnlyList<RiftTypeDefinition> Definitions => _definitions;
    /// <summary>같은 실제 SHP 저장소를 추가 UI에 빌린다. 새 파일 적재나 사본을 만들지 않는다.</summary>
    internal ShapeDatabase Shapes => _shapes;
    /// <summary>원문 내장 슬롯과 자산 슬롯의 경계다.</summary>
    private const int FirstAssetType = 70;
    /// <summary>현재 로딩 순서의 원문 기준점/논리 수와 물리 블록만 한 번 연결한다.</summary>
    public MarkupGameAssets(RiftTypeTable types, IReadOnlyList<RiftTypeDefinition> definitions, ShapeDatabase shapes)
    {
        _types = types ?? throw new ArgumentNullException(nameof(types)); _definitions = definitions ?? throw new ArgumentNullException(nameof(definitions)); _shapes = shapes ?? throw new ArgumentNullException(nameof(shapes));
        var bindings = new List<MarkupAssetBinding>();
        // 내장 무자산 슬롯에는 임의 Shape를 만들지 않고 최초 자산만 연결한다.
        for (int index = 0; index < definitions.Count; index++)
        {
            int type = checked(FirstAssetType + index); if (type >= types.Types.Count || index >= shapes.Blocks().Count) throw new ArgumentException("Markup 실제 자산 바인딩 범위 오류");
            var record = types.Types[type]; bindings.Add(new((uint)type, index, definitions[index].Clusters.Count, record.HotspotX, record.HotspotY));
        }
        Pictures = new(shapes, TypeCount, bindings);
    }
    /// <summary>현재 이름을 실제 최초 타입 표에서 찾으며 미등록은-1이다.</summary>
    public int FindType(string name) => _types.Find(name);
    /// <summary>유효한 원문 슬롯만 허용하며 내장/후행 무자산 타입은 빈 선택 필드를 반환한다.</summary>
    public StyleTextMarkupType Type(int type)
    {
        if (type < 0 || type >= TypeCount) throw new ArgumentOutOfRangeException(nameof(type), "스타일 실제 타입 범위 초과"); var view = new StyleTextMarkupType(); int index = type - FirstAssetType; if (index < 0 || index >= _definitions.Count) return view;
        var definition = _definitions[index]; view.FrameCount = definition.Clusters.Count; view.DefaultFrame = definition.SpecialFrames.DefaultFrame; view.GumpFrame = definition.SpecialFrames.GumpFrame;
        var frames = definition.FrameTable(); view.LetterKinds = frames.LetterKinds();
        // 원문 A~P 첫 인덱스를 보존하고 없는 글자를0으로 바꾸지 않는다.
        for (int letter = 0; letter < view.First.Length; letter++) view.First[letter] = frames.Run((byte)('A' + letter)).First;
        return view;
    }
    /// <summary>제한 그림에서만 같은 실제 헤더+1a의 signed 높이를 읽고 무자산/범위 밖은null이다.</summary>
    public short? FrameHeight(int type, int frame)
    {
        int index = type - FirstAssetType; if (type < FirstAssetType || frame < 0 || index >= _definitions.Count || frame >= _definitions[index].Clusters.Count) return null;
        return BinaryPrimitives.ReadInt16LittleEndian(_shapes.SquidHeader(index, frame).AsSpan(0x1a, 2));
    }
    /// <summary>기존 팔레트/소리 장치를 보존하고 같은 최초 자산 조회만 연결한다.</summary>
    public void BindStyle(StyleTextMarkupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); hooks.FindType = FindType; hooks.Type = Type; hooks.TypeCount = () => TypeCount; hooks.FrameHeight = FrameHeight;
    }
}
