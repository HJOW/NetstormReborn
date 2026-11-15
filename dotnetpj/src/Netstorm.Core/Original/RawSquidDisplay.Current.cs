// 실제 장기 표시는 각 원문 투영/clip 위치에서 현재 카메라 장치를 다시 조회한다.
namespace Netstorm.Core.Original;

/// <summary>독립 SetView의 값형 입력과 실제 현재 뷰 getter를 구분한다.</summary>
public sealed partial class SquidDisplay
{
    /// <summary>현재 Client 또는 짧은 World의 실제 카메라 조회다.</summary>
    private Func<SquidDisplayView>? _currentView;

    /// <summary>현재 getter만 연결하며 null이면 기존 독립 SetView 사본으로 돌아간다. 카메라/그리기는 실행하지 않는다.</summary>
    public void BindView(Func<SquidDisplayView>? view) => _currentView = view;

    /// <summary>선택한 함수 사본을 호출하고 구조상 null인 현재 장치를 기본 뷰로 숨기지 않는다.</summary>
    private SquidDisplayView CurrentView()
    {
        Func<SquidDisplayView>? read = _currentView;
        return read is null ? _view : read() ?? throw new InvalidOperationException("현재 표시 뷰의 실제 값 누락");
    }
}
