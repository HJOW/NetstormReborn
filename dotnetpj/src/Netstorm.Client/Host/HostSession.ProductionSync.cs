using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // Source의 실제 Session 설치 관문과 같은 생산 동기화 본체다.

/// <summary>생산 UI가 현재 Session의 준비된 Deck/기술/내용물만 사용하게 한다.</summary>
public sealed partial class HostSession
{
    /// <summary>전체 Source 적재/이동/커서 설치가 끝난 실제 공개 값이다. 현재 부분 적재는 이 값을 쓰지 않는다.</summary>
    private bool _nativeLoaded = false;
    /// <summary>기존 Editor 후반 UI 설치의 한정 동기 범위다. 실제 설치 부모 연결 전에는 BSS0이다.</summary>
    private bool _editorUiInstalling = false;
    /// <summary>파일 전 State1/2/6 HUD 설치의 한정 동기 범위다. 실제 prefix 부모 연결 전에는 BSS0이다.</summary>
    private bool _editorPrefixUiInstalling = false;
    /// <summary>같은 실제 Deck/물리/검색 전역을 사용하는 Source43df80이다.</summary>
    private RawProductionSync? _worldProductionSync;
    /// <summary>실제 Session 커서 부모의 Cancel 장치다. 누락을 빈 성공으로 바꾸지 않는다.</summary>
    private Action? _productionCancelCursor;

    /// <summary>Source Cursor getter와 같은 설치 관문 뒤 실제 커서 부모만 호출한다.</summary>
    public void CancelProductionCursor()
    {
        if (!_nativeLoaded && !_editorUiInstalling) throw new InvalidOperationException("설치 전 실제 커서 요청");
        (_productionCancelCursor ?? throw new InvalidOperationException("생산 동기화 실제 커서 미설치"))();
    }

    /// <summary>전체 loaded 또는 실제 두 Editor UI 동기 범위에서만 같은 현재 생산 본체를 호출한다.</summary>
    public void SynchronizeProductionContents()
    {
        if (!_nativeLoaded && !_editorUiInstalling && !_editorPrefixUiInstalling) throw new InvalidOperationException("설치 전 생산 패널 갱신");
        (_worldProductionSync ?? throw new InvalidOperationException("Session 실제 생산 동기화 본체 미설치")).Synchronize();
    }

    /// <summary>기존 Source 생산 모듈 준비 위치에서 같은 실제 장치로 본체만 만들며 loaded를 공개하지 않는다.</summary>
    private RawProductionSync CreateProductionSync(RawPlayerWorld player, RawProductionDeck deck, RawCarrierPreDestroy carrier, RawContainedCreate create, IReadOnlyList<OriginalTypeRecord> types, ContainedFinderState finder)
    {
        return new(Pool, types, finder, new() { LocalOwner = () => MissionPlayer.BattleOwner, FullDeck = () => Gameplay.FullDeckBox.Value }, new()
        {
            Deck = () => deck.Bytes(deck.Get(MissionPlayer.TemplateOwner)), Technology = player.CurrentTechnology,
            Temple = owner =>
            {
                if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "생산 동기화 현재 Player 범위");
                return BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan((int)owner * 180 + 0x7c)) != 0;
            },
            CancelCursor = CancelProductionCursor, Destroy = (sid, flags) => DestroyWorldObject(unchecked((int)sid), flags),
            FindKind = (parent, mask, kind, index) =>
            {
                int form = carrier.FindContained(unchecked((int)parent), mask, kind, index);
                return form == 0 ? 0u : BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(form)[18..]);
            },
            Create = (parent, kind, quantity, flags, attach, quality) => unchecked((uint)create.Create(unchecked((int)parent), kind, quantity, flags, attach, quality)),
            Assertion = text => Console.Error.WriteLine("생산 동기화 원본 진단: " + text),
        });
    }
}
