// 미션 저장소의 타일 묶음·일반 자산 수집기를 세션 수명에 연결한다.
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// 실제 지도 영역의 타일·객체 수집을 같은 세션 카메라·저장소에서 실행한다.
public sealed partial class HostSession
{
    /// <summary>수집기가 빌린 공간 해시다. 저장소가 바뀌면 수집기를 다시 만든다.</summary>
    private RawSquidHash? _worldHash;
    /// <summary>수집기가 빌린 SID 풀이다.</summary>
    private RawSidPool? _worldPool;
    /// <summary>수집한 화면 높이다. 바뀌면 수집기를 다시 만든다.</summary>
    private int _worldScreenHeight;
    /// <summary>수집 viewport의 실제 지도 영역 getter다. 미설치를 화면 전체로 대체하지 않는다.</summary>
    private Func<ScreenRect>? _worldMapView;
    /// <summary>수집 viewport의 실제 화면 가로다.</summary>
    private int _worldScreenWidth;
    /// <summary>같은 저장소의 타일 행별 묶음 수집기다.</summary>
    private RawTileGroupCollection? _worldTiles;
    /// <summary>같은 저장소의 일반 자산 수집기다.</summary>
    private RawObjectCollection? _worldObjects;

    /// <summary>같은 저장소·화면 크기로 타일·객체 수집기를 조립한다. 저장소·높이가 바뀌면 다시 만든다.</summary>
    /// <param name="hash">미션 공간 해시다.</param>
    /// <param name="pool">미션 SID 풀이다.</param>
    /// <param name="types">미션 타입 표다.</param>
    /// <param name="shapes">미션 셰이프 자료다.</param>
    /// <param name="mapView">실제 지도 영역 getter다.</param>
    /// <param name="screenWidth">수집 행 제한의 실제 화면 가로다.</param>
    /// <param name="screenHeight">수집 행 제한의 실제 화면 세로다.</param>
    /// <remarks>2026-11-26 1.068에서 R5-2d3 수집 결선용으로 추가했다. 근거: cpppj ClientRendererFrame.cpp의 Collect가 같은 Pool/해시/타입/셰이프로 수집기를 다시 만드는 결선이다. 배경·제출·매 프레임 구동은 R5-3에서 잇는다.</remarks>
    public void BuildWorldDrawingDevices(RawSquidHash hash, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<SquidDisplayShape> shapes, Func<ScreenRect> mapView, int screenWidth, int screenHeight)
    {
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(shapes);
        ArgumentNullException.ThrowIfNull(mapView);
        if (!ReferenceEquals(hash, _worldHash) || !ReferenceEquals(pool, _worldPool) || screenHeight != _worldScreenHeight)
        {
            // 생략된 상태(default)는 0 초기화라 토큰 영역 검사에 걸리므로 실제 높이를 넣어 표준 상태로 만든다
            _worldTiles = new RawTileGroupCollection(hash, pool, new RawTileGroupState(ScreenHeight: screenHeight));
            _worldObjects = new RawObjectCollection(hash, pool, types, shapes);
            _worldHash = hash;
            _worldPool = pool;
            _worldScreenHeight = screenHeight;
        }
        _worldMapView = mapView;
        _worldScreenWidth = screenWidth;
    }

    /// <summary>실제 지도 영역 전역으로 타일 묶음·일반 자산을 순서대로 수집한다.</summary>
    /// <param name="clip">실제 dirty 사각형이다.</param>
    /// <returns>순차 병합한 표시 항목이다. 배경 덮음은 타일 수집기가 정한다.</returns>
    public RawDrawCollectionResult CollectWorldDrawing(ScreenRect clip)
    {
        RawTileGroupCollection tiles = _worldTiles ?? throw new InvalidOperationException("월드 수집 장치 미연결");
        RawObjectCollection objects = _worldObjects ?? throw new InvalidOperationException("월드 수집 장치 미연결");
        Func<ScreenRect> mapView = _worldMapView ?? throw new InvalidOperationException("월드 수집 지도 영역 미설정(R5: 전체 viewport 대체 없음)");
        RenderWorldRect visible = RawRendererViewport.CollectionViewport(clip, mapView(), Gameplay.Camera.Snapshot);
        int rowLimit = RawRendererViewport.CollectionRowLimit(_worldScreenWidth, _worldScreenHeight);
        var hooks = new RawDrawCollectionHooks
        {
            Groups = (RenderWorldRect view, int limit, ref bool covered) =>
            {
                bool local = false;
                IReadOnlyList<RawTileGroupView> found = tiles.Collect(view, limit, out local);
                covered = local;
                return found;
            },
            Objects = (RenderWorldRect view, int limit) =>
            {
                var work = new RenderCollectionEntry[RawDrawSubmission.Capacity];
                int count = objects.Collect(view, work, limit);
                return work[..count];
            },
        };
        return RawDrawSubmission.CollectDrawEntries(new(clip, visible, visible, 0, rowLimit), hooks);
    }
}
