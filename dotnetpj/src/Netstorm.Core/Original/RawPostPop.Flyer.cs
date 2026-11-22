// 원문 비행 전체 몸체의 같은 Pool 등록과 실제 VT 선택을 분리한다.
namespace Netstorm.Core.Original;

/// <summary>SquidPostPop.cpp의 SetFlyerPostPop/HandlesFlyer를 같은 공통 부모에 연결한다.</summary>
public sealed partial class RawPostPop
{
    /// <summary>같은 풀의 맨 오 워/훨리기그/더스트 데빌 전체 후처리다.</summary>
    private RawFlyerPostPop? _flyerPostPop;
    /// <summary>같은 Pool의 실제 비행 몸체만 등록하며 null이면 현재 대여를 반납한다.</summary>
    public void SetFlyerPostPop(RawFlyerPostPop? body)
    {
        if (body is not null && !ReferenceEquals(body.Pool, _pool)) throw new ArgumentException("비행 PostPop/공통 풀 불일치", nameof(body));
        _flyerPostPop = body;
    }
    /// <summary>등록된 실제 세 비행 VT만 전체 몸체로 분배한다.</summary>
    public bool HandlesFlyer(int sid) => _flyerPostPop is not null && _flyerPostPop.Handles(sid);
}
