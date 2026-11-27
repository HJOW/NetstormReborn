using Netstorm.Client.Gump;
using Netstorm.Core.Original;

// 생산 UI의 실제 GumpForm 생성과 객체 부착을 같은 장기 물리에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>원문의 현재 UI 등록 ID와 공유 공장으로 생산 Form을 만든다.</summary>
public static class HostProductionFormBinding
{
    /// <summary>Form 생성 뒤 현재 노드의 등록 ID를 쓰고 대상의 하위 WORD에 부착한다.</summary>
    /// <param name="storage">현재 Client가 소유한 Native 장기 물리다.</param>
    /// <param name="tree">같은 생산 UI의 실제 공통 트리다.</param>
    /// <param name="token">현재 생산 노드 토큰이다.</param>
    /// <param name="parent">원문처럼 하위 WORD를 사용하는 대상 객체다.</param>
    /// <returns>생성해 부착한 실제 Form 번호다.</returns>
    public static uint Bind(RawSurfaceStorage storage, GumpButtonTree tree, uint token, uint parent)
    {
        ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(tree);
        if (!storage.UsesNativeTransmissionDevices()) throw new InvalidOperationException("생산 Form의 실제 Native 물리 미연결");
        RawGumpFormHost forms = storage.GumpForms();
        int form = forms.Create(2);
        GumpCreateNode node = tree.Node(token) ?? throw new InvalidOperationException("생산 Form 실제 Gump 없음");
        forms.SetOwner(form, node.Id);
        forms.Attach(form, unchecked((ushort)parent), 0);
        return unchecked((uint)form);
    }
}
