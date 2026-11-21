// 현재 실제 종속 UI/링크/공통 Pop과 전송을 자산 가상50 부착에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawSurfaceWorld.cpp의004ad330 자산 부착 본체다.</summary>
public sealed partial class RawSurfaceWorld
{
    /// <summary>원문 진단 복귀 뒤 좌표 저장→현재 UI통지→실제 공통 부착/Pop을 수행하고 끝에서만 변경을 기록한다.</summary>
    public void AttachBaseAsset(int sid, int parent, uint flags)
    {
        RawSquidBaseAttach.Attach(_storage.Pool, sid, parent, flags, new()
        {
            Assertion = _storage.AssertDisplay, Genus = type => _types[type].Flags2, ContentType = () => CurrentContainedUnpop().ContentFormType(),
            NotifyUi = selected => CurrentContainedUnpop().NotifyUi(selected),
            PostPop = () => _postPop ?? throw new InvalidOperationException("종속 자산의 실제 후처리 미연결"),
            Transmit = (selected, value) => (_squidTransmission ?? throw new InvalidOperationException("종속 자산의 실제 Squid 전송 미연결")).Pop(selected, value),
        });
        _changed = true;
    }
}
