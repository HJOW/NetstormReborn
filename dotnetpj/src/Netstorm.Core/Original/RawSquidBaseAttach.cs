using System.Buffers.Binary;

// World와 실제 Host가 같은 자산 가상50 본체와 선택된 현재 장치를 공유한다.
namespace Netstorm.Core.Original;

/// <summary>자산 부착이 원문 위치에서 선택하는 실제 종속 UI/TYPE/후처리/전송 장치다.</summary>
public sealed class SquidBaseAttachHooks
{
    /// <summary>좌표 접두 뒤 현재 종속 UI를 통지한다.</summary>
    public Action<int>? NotifyUi { get; init; }
    /// <summary>현재 raw TYPE의 flags2다.</summary>
    public Func<byte, uint>? Genus { get; init; }
    /// <summary>현재 전체 contentFormType DWORD다.</summary>
    public Func<uint>? ContentType { get; init; }
    /// <summary>공통 Pop이 선택하는 같은 실제 후처리 부모다.</summary>
    public Func<RawPostPop>? PostPop { get; init; }
    /// <summary>같은 장기 버퍼/캐시를 쓰는 실제 Squid Pop 전송이다.</summary>
    public Action<int, uint>? Transmit { get; init; }
    /// <summary>현재 Client의 복귀 가능한 원문 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; init; }
}

/// <summary>RawSurfaceWorld.cpp AttachBaseAsset의 동일 본체를 별도 풀/World 없이 실행한다.</summary>
public static class RawSquidBaseAttach
{
    /// <summary>도달한 현재 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("자산 부착의 선택된 실제 장치 미연결");
    /// <summary>PATCH 진단→좌표 접두→UI→실제 링크/공통 Pop 순서를 보존한다. 외곽 changed는 정상 반환 뒤 호출자가 기록한다.</summary>
    public static void Attach(RawSidPool pool, int sid, int parent, uint flags, SquidBaseAttachHooks hooks)
    {
        if (pool.Edition != OriginalEdition.Patch1078) throw new InvalidOperationException("자산 종속 수신의 PATCH 가상 본체 계약");
        Need(hooks.Assertion)("0", @"\Ns\O\Squid.cpp", 0x72a);
        Span<byte> raw = pool.RawSlot(sid); BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(18, 4), 0);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(14, 4), parent); Need(hooks.NotifyUi)(sid);
        RawFormAttach.Attach(pool, sid, parent, flags, new()
        {
            Assertion = (text, file, line) => Need(hooks.Assertion)(text, file, line), Genus = type => Need(hooks.Genus)(type), ContentType = () => Need(hooks.ContentType)(),
            SetParent = (form, _, into) => RawContainedLink.CommonContainedSetParent(pool, form, into),
            SetPrevious = (form, table, previous) => RawContainedLink.CommonContainedSetPreviousAtTable(pool, form, table, previous),
            Pop = (form, requested) => RawFormPop.PopCommonForm(pool, form, requested, new()
            {
                Assertion = (text, file, line) => Need(hooks.Assertion)(text, file, line), Server = () => pool.IsServer,
                Depth = () => Need(hooks.PostPop)().ActivationDepth, SetDepth = value => Need(hooks.PostPop)().ActivationDepth = value,
                PostPop = (selected, _, value) => Need(hooks.PostPop)().PostPopVirtual(selected, value),
                Transmit = (selected, _, _, _, value) => Need(hooks.Transmit)(selected, value),
            }),
        });
    }
}
