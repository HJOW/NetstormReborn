// 실제 World의 현재 공간 해제는 같은 Client 진단/TYPE와 현재 Session 종속 분리를 빌린다.
namespace Netstorm.Core.Original;

/// <summary>World의 같은 공간 부모에 현재 진단과 실제 분리 종단을 연결한다.</summary>
public sealed partial class RawSurfaceWorld
{
    /// <summary>현재 TYPE와 checking을 공유하고 분리는 소비 시점의 Session/기본 종단을 선택한다.</summary>
    private void BindCurrentUnpop()
    {
        RawUnpop unpop = _unpop ?? throw new InvalidOperationException("현재 Unpop 부모 미연결");
        // 장기 실행기는 Storage 진단/현재 종단을 유지하며 짧은 World 콜백을 보관하지 않는다.
        if (_storage.Types.Count == _types.Count && _display is not null) return;
        unpop.BindDiagnostics(new() { Types = _types, Checking = _storage.Checking(), Assertion = _storage.AssertDisplay });
        unpop.BindContainedUnpop(sid => CurrentContainedUnpop().Unpop(sid));
    }
}
