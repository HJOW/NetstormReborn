using Netstorm.Core.Original;

// 실제 TYPE 번호에 맞춘 일반 객체 표시용 자산 정보를 구성한다.
namespace Netstorm.Client.Display;

/// <summary>원본 자산 정의와 SHP 헤더를 SpriteDraw가 쓰는 동일 TYPE 순서로 준비한다.</summary>
public static class SpriteAssetViews
{
    /// <summary>파일 자산 TYPE의 시작 번호다. 앞의 내장 TYPE도 결과에서 보존한다.</summary>
    private const int FirstAssetType = 70;

    /// <summary>같은 TYPE 순서의 셰이프 사본과 기본 프레임·막대 속성을 만든다.</summary>
    /// <param name="shapes">내장 TYPE을 포함하는 실제 SHP 헤더 표다.</param>
    /// <param name="definitions">파일 자산의 원본 TYPE 순서 정의다.</param>
    /// <returns>SpriteDraw에 전달할 TYPE별 독립 표시 표다.</returns>
    /// <remarks>cpppj SpriteDraw.cpp의 MakeSpriteAssetViews와 동일한 기본 프레임·숫자 절삭을 사용한다. 2026-11-27 일반 객체 출력 준비에 추가했다.</remarks>
    public static SpriteTypeView[] Build(IReadOnlyList<SquidDisplayShape> shapes, IReadOnlyList<RiftTypeDefinition> definitions)
    {
        ArgumentNullException.ThrowIfNull(shapes);
        ArgumentNullException.ThrowIfNull(definitions);
        if (shapes.Count < FirstAssetType || definitions.Count > shapes.Count - FirstAssetType)
            throw new ArgumentException("Sprite 자산 정의와 TYPE 셰이프 수 불일치");
        var result = new SpriteTypeView[shapes.Count];
        // 내장 TYPE을 포함해 각 표시 셰이프를 독립 사본으로 보존한다.
        for (int index = 0; index < shapes.Count; index++)
            result[index] = new SpriteTypeView { Shape = (shapes[index] ?? throw new ArgumentException("Sprite 셰이프 누락", nameof(shapes))).Clone() };
        // 파일 자산 정의만 원본 TYPE 번호에 대응시켜 막대 입력을 설정한다.
        for (int index = 0; index < definitions.Count; index++)
        {
            RiftTypeDefinition definition = definitions[index] ?? throw new ArgumentException("Sprite TYPE 정의 누락", nameof(definitions));
            SpriteTypeView view = result[FirstAssetType + index];
            view.GumpFrame = unchecked((uint)definition.SpecialFrames.DefaultFrame);
            view.Capacity = SpriteDraw.Truncate(definition.Number("casttime") ?? 0);
            view.ConstructionTime = SpriteDraw.Truncate(definition.Number("praytime") ?? 0);
            view.Duration = SpriteDraw.Truncate(definition.Number("effecttime") ?? 0);
        }
        return result;
    }

    /// <summary>그림자 별칭을 매 호출에 다시 읽어 실제 SHP 블록을 선택하는 조회기를 만든다.</summary>
    /// <param name="database">파일 자산과 같은 실제 SHP 저장소다.</param>
    /// <param name="assetCount">TYPE 70부터 순서대로 놓인 자산 블록 수다.</param>
    /// <param name="state">현재 그림자 별칭을 보유하는 표시 상태다.</param>
    /// <returns>타입을 실제 저장소와 블록으로 변환하는 조회기다.</returns>
    /// <remarks>cpppj SpriteDraw.cpp의 MakeSpriteAssetShapes를 따른다. 2026-11-27 일반 객체 출력 준비에 추가했다.</remarks>
    public static Func<uint, PreviewShapeBinding> BindShapes(ShapeDatabase database, int assetCount, SpriteDrawState state)
    {
        ArgumentNullException.ThrowIfNull(database);
        ArgumentNullException.ThrowIfNull(state);
        if (assetCount < 0) throw new ArgumentOutOfRangeException(nameof(assetCount));
        if (assetCount > database.Blocks().Count) throw new ArgumentException("Sprite 자산 블록 수 부족", nameof(assetCount));
        return type =>
        {
            if (type == state.ShadowAliasType && state.ShadowShapeType is uint alias) type = alias;
            if (type < FirstAssetType || type - FirstAssetType >= (uint)assetCount)
                throw new ArgumentOutOfRangeException(nameof(type), "Sprite 자산 TYPE 범위 오류");
            return new PreviewShapeBinding(database, checked((int)(type - FirstAssetType)));
        };
    }
}
