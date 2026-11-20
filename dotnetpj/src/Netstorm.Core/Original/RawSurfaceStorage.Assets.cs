// 최초 자산 owner와 수정 가능한 현재 월드 사본의 수명/값을 구분한다.
namespace Netstorm.Core.Original;

/// <summary>실제 자산 owner의 읽기 전용 최초 TYPE 표 경계다. Native 자산 owner는 로딩 뒤 이 표를 변경하지 않는다.</summary>
public interface IRawSurfaceAssetSource
{
    /// <summary>GameAssets::TypeTable().Types()의 실제 최초 로딩 표다. 현재 Storage.Types와 다른 owner이며 전체 자산 번호 체계를 사용한다.</summary>
    IReadOnlyList<OriginalTypeRecord> LoadedTypes { get; }
}

/// <summary>수정 가능한 TYPE/프레임/셰이프 값 사본과 원래 자산 정체성을 별도로 유지한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>호출자가 명시 전달한 실제 최초 자산 owner다. 자산 사본이 있다는 이유로 합성하지 않는다.</summary>
    private IRawSurfaceAssetSource? _assetSource;
    /// <summary>빈 표도 원문 최초 초기화 완료와 미설치를 구분하는 호스트 수명 표식이다.</summary>
    private bool _assetTablesInitialized;

    /// <summary>실제 최초 자산 정체성이며 독립 typed 저장소는 null이다. 조회 자체는 TYPE/파일을 읽지 않는다.</summary>
    public IRawSurfaceAssetSource? Source() => _assetSource;
}
